use crate::{socket::{ISocket, build_thread}, event::Event, AnyError};
use std::{
	sync::{
		atomic::{self, AtomicBool},
		Arc,
	},
	thread::{self, JoinHandle},
	time::{Duration, Instant},
};

pub struct Queue {
	receiver: crossbeam_channel::Receiver<Event>,
	sender: crossbeam_channel::Sender<Event>,
	thread_poll_events: Option<JoinHandle<()>>,
}

impl Queue {
	pub(crate) fn new(
		name: String,
		socket: Box<dyn ISocket + Send>,
		exit_flag: &Arc<AtomicBool>,
	) -> Result<Self, AnyError> {
		let (sender, receiver) = crossbeam_channel::unbounded();

		let laminar_to_socknet_sender = sender.clone();
		let thread_exit_flag = exit_flag.clone();
		let thread_poll_events = Some(build_thread(name, move || {
			Self::poll_events(socket, laminar_to_socknet_sender, thread_exit_flag);
		})?);

		Ok(Self {
			receiver,
			sender,
			thread_poll_events,
		})
	}

	fn poll_events(
		mut socket: Box<dyn ISocket + Send>,
		laminar_to_socknet_sender: crossbeam_channel::Sender<Event>,
		exit_flag: Arc<AtomicBool>,
	) {
		use crossbeam_channel::{TryRecvError, TrySendError};
		// equivalent to `laminar::Socket::start_polling`, with the addition of moving packets into the destination queue
		loop {
			if exit_flag.load(atomic::Ordering::Relaxed) {
				break;
			}
			socket.manual_poll(Instant::now());
			match socket.get_event_receiver().try_recv() {
				// found event, add to queue and continue the loop
				Ok(event) => {
					let event = event.into();
					match laminar_to_socknet_sender.try_send(event) {
						Ok(_) => {}                            // success case is no-op
						Err(TrySendError::Full(_packet)) => {} // no-op, the channel is unbounded
						Err(TrySendError::Disconnected(_packet)) => break,
					}
				}
				// no events, continue the loop after a short nap
				Err(TryRecvError::Empty) => thread::sleep(Duration::from_millis(1)),
				// If disconnected, then kill the thread
				Err(TryRecvError::Disconnected) => break,
			}
		}
		log::debug!(target: crate::LOG, "Polling thread has concluded");
	}

	pub fn channel(&self) -> &crossbeam_channel::Receiver<Event> {
		&self.receiver
	}

	pub fn sender(&self) -> &crossbeam_channel::Sender<Event> {
		&self.sender
	}
}

impl Drop for Queue {
	fn drop(&mut self) {
		self.thread_poll_events.take().unwrap().join().unwrap();
	}
}

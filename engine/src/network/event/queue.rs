use crate::{
	network::{build_thread, event::Event},
	utility::AnyError,
};
use std::{
	collections::VecDeque,
	sync,
	thread::{self, JoinHandle},
	time::{Duration, Instant},
};

pub struct Queue {
	queue: sync::Arc<sync::Mutex<VecDeque<Event>>>,
	thread_poll_events: Option<JoinHandle<()>>,
}

impl Queue {
	pub(crate) fn new(name: String, socket: laminar::Socket) -> Result<Self, AnyError> {
		let queue = sync::Arc::new(sync::Mutex::new(VecDeque::new()));

		let event_queue = queue.clone();
		let thread_poll_events = Some(build_thread(name, move || {
			Self::poll_events(socket, event_queue);
		})?);

		Ok(Self {
			queue,
			thread_poll_events,
		})
	}

	fn poll_events(
		mut socket: laminar::Socket,
		destination_queue: sync::Arc<sync::Mutex<VecDeque<Event>>>,
	) {
		use crossbeam_channel::TryRecvError;
		// equivalent to `laminar::Socket::start_polling`, with the addition of moving packets into the destination queue
		loop {
			socket.manual_poll(Instant::now());
			match socket.get_event_receiver().try_recv() {
				// found event, add to queue and continue the loop
				Ok(event) => {
					let event = event.into();
					destination_queue.lock().unwrap().push_back(event);
				}
				// no events, continue the loop after a short nap
				Err(TryRecvError::Empty) => thread::sleep(Duration::from_millis(1)),
				// If disconnected, then kill the thread
				Err(TryRecvError::Disconnected) => return,
			}
		}
	}

	pub fn handle(&self) -> &sync::Arc<sync::Mutex<VecDeque<Event>>> {
		&self.queue
	}
}

impl Drop for Queue {
	fn drop(&mut self) {
		self.thread_poll_events.take().unwrap().join().unwrap();
	}
}

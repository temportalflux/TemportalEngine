use crate::{
	network::{build_thread, packet},
	utility::AnyError,
};
use std::{
	collections::VecDeque,
	sync,
	thread::{self, JoinHandle},
	time::Duration,
};

pub struct Queue {
	queue: sync::Arc<sync::Mutex<VecDeque<packet::Packet>>>,
	thread_send_packets: Option<JoinHandle<()>>,
}

impl Queue {
	pub(crate) fn new(
		name: String,
		sender: crossbeam_channel::Sender<laminar::Packet>,
	) -> Result<Self, AnyError> {
		let queue = sync::Arc::new(sync::Mutex::new(VecDeque::new()));

		let packet_queue = queue.clone();
		let thread_send_packets = Some(build_thread(name, move || {
			Self::send_packets(packet_queue, sender);
		})?);

		Ok(Self {
			queue,
			thread_send_packets,
		})
	}

	fn send_packets(
		queue: sync::Arc<sync::Mutex<VecDeque<packet::Packet>>>,
		sender: crossbeam_channel::Sender<laminar::Packet>,
	) {
		use crossbeam_channel::TrySendError;
		let mut next_packet: Option<laminar::Packet> = None;
		loop {
			if next_packet.is_none() {
				next_packet = queue
					.lock()
					.unwrap()
					.pop_front()
					.map(|packet| packet.into());
			}
			if let Some(packet) = next_packet.take() {
				match sender.try_send(packet) {
					Ok(_) => {} // success case is no-op
					Err(TrySendError::Full(packet)) => {
						// put the packet back and wait for next loop to try to send again
						next_packet = Some(packet);
					}
					Err(TrySendError::Disconnected(_packet)) => return,
				}
			}
			thread::sleep(Duration::from_millis(1));
		}
	}

	pub fn handle(&self) -> &sync::Arc<sync::Mutex<VecDeque<packet::Packet>>> {
		&self.queue
	}
}

impl Drop for Queue {
	fn drop(&mut self) {
		self.thread_send_packets.take().unwrap().join().unwrap();
	}
}

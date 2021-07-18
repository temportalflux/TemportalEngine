use crate::{network::event, utility::AnyError};
use std::{
	collections::VecDeque,
	net::{IpAddr, Ipv4Addr, SocketAddr},
	sync,
	thread::{self, JoinHandle},
};

pub type SocketEventQueue = sync::Arc<sync::Mutex<VecDeque<event::Event>>>;

pub(crate) fn build_thread<F, T>(name: String, f: F) -> std::io::Result<JoinHandle<T>>
where
	F: FnOnce() -> T,
	F: Send + 'static,
	T: Send + 'static,
{
	thread::Builder::new().name(name).spawn(f)
}

pub struct Socket {
	packet_sender: crossbeam_channel::Sender<laminar::Packet>,
	event_queue: event::Queue,
	address: SocketAddr,
}

impl Socket {
	fn make_thread_name(address: &SocketAddr) -> String {
		format!("network:socket({})", address)
	}

	/// Creates a socket on a given port
	pub fn new(port: u16) -> Result<Self, AnyError> {
		let address = SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), port);
		let name = Self::make_thread_name(&address);
		let socket = laminar::Socket::bind(address)?;
		let packet_sender = socket.get_packet_sender();
		let event_queue = event::Queue::new(format!("{}:reader", name), socket)?;
		Ok(Self {
			address,
			packet_sender,
			event_queue,
		})
	}

	pub fn name(&self) -> String {
		Self::make_thread_name(&self.address)
	}

	pub fn port(&self) -> u16 {
		self.address.port()
	}

	pub fn create_reception_queue(&self) -> SocketEventQueue {
		self.event_queue.handle().clone()
	}
}

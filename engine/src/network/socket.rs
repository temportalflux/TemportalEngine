use crate::{
	network::{event, packet},
	utility::AnyError,
};
use std::{
	collections::VecDeque,
	net::{IpAddr, Ipv4Addr, SocketAddr},
	sync,
	thread::{self, JoinHandle},
	time::Duration,
};

pub type SocketIncomingQueue = sync::Arc<sync::Mutex<VecDeque<event::Event>>>;
pub type SocketOutgoingQueue = sync::Arc<sync::Mutex<VecDeque<packet::Packet>>>;

pub(crate) fn build_thread<F, T>(name: String, f: F) -> std::io::Result<JoinHandle<T>>
where
	F: FnOnce() -> T,
	F: Send + 'static,
	T: Send + 'static,
{
	thread::Builder::new().name(name).spawn(f)
}

pub struct Socket {
	outgoing_queue: packet::Queue,
	incoming_queue: event::Queue,
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
		let config = laminar::Config {
			idle_connection_timeout: Duration::from_secs(5),
			heartbeat_interval: Some(Duration::from_millis(2 * 1000)),
			..Default::default()
		};
		let socket = laminar::Socket::bind_with_config(address, config)?;
		let outgoing_queue =
			packet::Queue::new(format!("{}:outgoing", name), socket.get_packet_sender())?;
		let incoming_queue = event::Queue::new(format!("{}:incoming", name), socket)?;
		Ok(Self {
			address,
			incoming_queue,
			outgoing_queue,
		})
	}

	pub fn name(&self) -> String {
		Self::make_thread_name(&self.address)
	}

	pub fn port(&self) -> u16 {
		self.address.port()
	}

	pub fn create_incoming_queue(&self) -> SocketIncomingQueue {
		self.incoming_queue.handle().clone()
	}

	pub fn create_outgoing_queue(&self) -> SocketOutgoingQueue {
		self.outgoing_queue.handle().clone()
	}
}

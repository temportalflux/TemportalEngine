use crate::{event, packet, AnyError};
use std::{
	collections::VecDeque,
	net::{IpAddr, Ipv4Addr, SocketAddr},
	sync::{atomic::AtomicBool, Arc, Mutex},
	thread::{self, JoinHandle},
	time::Duration,
};

pub type SocketIncomingQueue = Arc<Mutex<VecDeque<event::Event>>>;
pub type SocketOutgoingQueue = Arc<Mutex<VecDeque<packet::Packet>>>;

pub(crate) fn build_thread<F, T>(name: String, f: F) -> std::io::Result<JoinHandle<T>>
where
	F: FnOnce() -> T,
	F: Send + 'static,
	T: Send + 'static,
{
	thread::Builder::new().name(name).spawn(f)
}

pub fn start(
	port: u16,
	exit_flag: &Arc<AtomicBool>,
) -> Result<(packet::Queue, event::Queue), AnyError> {
	let address = SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), port);
	let name = format!("network:socket({})", address);
	let config = laminar::Config {
		idle_connection_timeout: Duration::from_secs(5),
		heartbeat_interval: Some(Duration::from_millis(2 * 1000)),
		..Default::default()
	};
	let socket = laminar::Socket::bind_with_config(address, config)?;
	let outgoing_queue = packet::Queue::new(
		format!("{}:outgoing", name),
		socket.get_packet_sender(),
		&exit_flag,
	)?;
	let incoming_queue = event::Queue::new(format!("{}:incoming", name), socket, &exit_flag)?;
	Ok((outgoing_queue, incoming_queue))
}

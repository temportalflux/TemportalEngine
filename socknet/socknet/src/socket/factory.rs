use crate::{backend, channel, AnyError};
use std::net::SocketAddr;

pub trait Factory {
	fn build(
		address: SocketAddr,
		config: backend::Config,
	) -> Result<Box<dyn ISocket + Send>, AnyError>;
}

pub trait ISocket {
	fn get_packet_sender(&self) -> channel::Sender<backend::Packet>;
	fn get_event_receiver(&self) -> channel::Receiver<backend::Event>;
	fn manual_poll(&mut self, time: std::time::Instant);
}

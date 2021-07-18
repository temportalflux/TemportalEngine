use crate::packet::Packet;
use std::net::SocketAddr;

#[derive(Debug)]
pub enum Event {
	Connected(SocketAddr),
	TimedOut(SocketAddr),
	Disconnected(SocketAddr),
	Packet(Packet),
}

impl From<laminar::SocketEvent> for Event {
	fn from(event: laminar::SocketEvent) -> Self {
		use laminar::SocketEvent::*;
		match event {
			Connect(address) => Self::Connected(address),
			Timeout(address) => Self::TimedOut(address),
			Disconnect(address) => Self::Disconnected(address),
			Packet(packet) => Self::Packet(packet.into()),
		}
	}
}

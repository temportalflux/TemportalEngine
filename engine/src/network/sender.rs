use super::{connection, packet};
use crate::utility::VoidResult;
use std::sync::{Arc, RwLock};

pub struct Sender {
	pub(super) connection_list: Arc<RwLock<connection::List>>,
	pub(super) receiver_event_sender: socknet::channel::Sender<socknet::event::Event>,
	pub(super) queue: socknet::packet::Queue,
}

impl Sender {
	pub fn stop(&self) -> VoidResult {
		self.receiver_event_sender
			.try_send(socknet::event::Event::Stop)?;
		Ok(())
	}

	/// Enqueues the packet to be sent in the sending thread
	pub fn send(&self, packet: packet::Packet) -> VoidResult {
		self.queue.channel().try_send(packet)?;
		Ok(())
	}

	pub fn send_to_server(&self, packet: packet::PacketBuilder) -> VoidResult {
		if let Ok(connection_list) = self.connection_list.read() {
			if let Some(connection) = connection_list.get_connection(&connection::Id(0)) {
				self.send(packet.with_address(connection.address)?.build())?;
			}
		}
		Ok(())
	}

	/// Enqueues a bunch of duplicates of the packet,
	/// one for each connection, to be sent in the sending thread.
	pub fn broadcast(&self, packet: packet::PacketBuilder) -> VoidResult {
		if let Ok(connection_list) = self.connection_list.read() {
			for conn in connection_list.iter() {
				self.send(packet.clone().with_address(conn.address)?.build())?;
			}
		}
		Ok(())
	}
}

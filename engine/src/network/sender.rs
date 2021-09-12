use super::{connection, mode, packet, LocalData, LOG};
use crate::utility::VoidResult;
use std::sync::{Arc, RwLock};

pub struct Sender {
	pub(super) connection_list: Arc<RwLock<connection::List>>,
	pub(super) receiver_event_sender: socknet::channel::Sender<socknet::event::Event>,
	pub(super) queue: socknet::packet::Queue,
	pub(super) local_data: LocalData,
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

	pub fn send_to_server(&self, mut packet: packet::PacketBuilder) -> VoidResult {
		if !self.local_data.is_client() {
			log::error!(
				target: LOG,
				"Can only send_to_server PacketKind({}) if running as a client.",
				packet.packet_kind()
			);
			return Ok(());
		}

		// i.e. is not a CotoS
		if self.local_data.is_dedicated(mode::Kind::Client) {
			if let Ok(connection_list) = self.connection_list.read() {
				if let Some(connection) = connection_list.get_connection(&connection::Id(0)) {
					packet.set_address(connection.address)?;
				}
			}
		}
		// is both a client and a server, so send to specifically our own address (first connection may not be ourself when CotoS).
		else {
			packet.set_address(self.local_data.address)?;
		}
		self.send(packet.build())?;
		Ok(())
	}

	/// Enqueues a bunch of duplicates of the packet,
	/// one for each connection, to be sent in the sending thread.
	pub fn broadcast(&self, packet: packet::PacketBuilder) -> VoidResult {
		if !self.local_data.is_server() {
			log::error!(
				target: LOG,
				"Can only broadcast PacketKind({}) if running as a server.",
				packet.packet_kind()
			);
			return Ok(());
		}
		if let Ok(connection_list) = self.connection_list.read() {
			for conn in connection_list.iter() {
				// broadcasts only come from the server,
				// and should never be sent to themselves if they are also a client
				if !self.local_data.is_local(&conn) {
					self.send(packet.clone().with_address(conn.address)?.build())?;
				}
			}
		}
		Ok(())
	}
}

impl Drop for Sender {
	fn drop(&mut self) {
		log::debug!(target: LOG, "Dropping network dispatch");
	}
}

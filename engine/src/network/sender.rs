use super::{connection, mode, packet, LocalData, LOG};
use crate::utility::VoidResult;
use std::{
	net::SocketAddr,
	sync::{Arc, RwLock},
};

pub struct Sender {
	pub(super) connection_list: Arc<RwLock<connection::List>>,
	pub(super) receiver_event_sender: socknet::channel::Sender<socknet::event::Event>,
	pub(super) queue: socknet::packet::Queue,
	pub(super) local_data: LocalData,
}

impl socknet::packet::AddressReference for Sender {
	fn local_address(&self) -> &SocketAddr {
		self.local_data.address()
	}

	fn active_addresses(&self) -> Vec<SocketAddr> {
		let connection_list = self.connection_list.read().unwrap();
		connection_list
			.iter()
			.map(|connection| connection.address.clone())
			.collect()
	}

	fn server_address(&self) -> SocketAddr {
		// i.e. is not a CotoS
		if self.local_data.is_dedicated(mode::Kind::Client) {
			match self.connection_list.read() {
				Ok(connection_list) => match connection_list.get_connection(&connection::Id(0)) {
					Some(connection) => connection.address.clone(),
					None => unimplemented!("There is no server connection"),
				},
				Err(_) => unimplemented!("Cannot read connection list"),
			}
		}
		// is both a client and a server, so send to specifically our own address (first connection may not be ourself when CotoS).
		else {
			self.local_data.address
		}
	}
}

impl Sender {
	pub fn stop(&self) -> VoidResult {
		self.receiver_event_sender
			.try_send(socknet::event::Event::Stop)?;
		Ok(())
	}

	/// Enqueues the packet to be sent in the sending thread
	pub fn send_packets(&self, builder: packet::PacketBuilder) -> VoidResult {
		for packet in builder.into_packets(self) {
			self.queue.channel().try_send(packet)?;
		}
		Ok(())
	}

	pub fn kick(&self, address: &SocketAddr) -> VoidResult {
		self.queue.kick(address.clone())?;
		Ok(())
	}
}

impl Drop for Sender {
	fn drop(&mut self) {
		log::debug!(target: LOG, "Dropping network dispatch");
	}
}

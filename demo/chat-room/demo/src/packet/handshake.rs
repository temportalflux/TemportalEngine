use crate::engine::{
	network::{
		self,
		connection::Connection,
		event, mode,
		packet::{Guarantee, Packet},
		packet_kind,
		processor::{EventProcessors, PacketProcessor, Processor},
		LocalData, Network,
	},
	utility::VoidResult,
};
use serde::{Deserialize, Serialize};

/// The very first packet sent to establish a connection between a client and server.
///
/// Sent by the connecting client in order to connect with a server.
/// Once the client receives the server's response (sending the same packet back again),
/// the backend heartbeat takes over to maintain the connection.
///
/// If the packet is dropped either on the way to the server or on the return trip,
/// the client's connection will timeout.
#[packet_kind(crate::engine::network)]
#[derive(Serialize, Deserialize)]
pub struct Handshake {
	pub display_name: String,
}

impl Handshake {
	pub fn register(builder: &mut network::Builder) {
		use mode::Kind::*;
		builder.register_bundle::<Handshake>(
			EventProcessors::default()
				.with(Server, SendBackToClient())
				.with(mode::Set::all(), SendBackToClient())
				.ignore(Client),
		);
	}
}

struct SendBackToClient();

impl Processor for SendBackToClient {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Handshake> for SendBackToClient {
	fn process_packet(
		&self,
		_kind: &event::Kind,
		data: &mut Handshake,
		connection: &Connection,
		guarantee: &Guarantee,
		local_data: &LocalData,
	) -> VoidResult {
		// Save the name of this connection for future messages
		if let Ok(mut history) = crate::MessageHistory::write() {
			history.add_pending_user(connection.address.clone(), data.display_name.clone());
			if connection.id.is_some() {
				history.confirm_user(&connection);
				if let Some(user_name) = history.get_user(&connection.id.unwrap()).cloned() {
					super::ConfirmUser::announce_arrival(local_data, &mut history, &user_name)?;
				}
			}
		}

		if !local_data.is_local(&connection) {
			Network::send_packets(
				Packet::builder()
					.with_address(connection.address)?
					.with_guarantee(*guarantee)
					.with_payload(data),
			)?;
		}
		Ok(())
	}
}

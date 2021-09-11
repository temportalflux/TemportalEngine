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
		kind: event::Kind,
		data: Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Handshake> for SendBackToClient {
	fn process_packet(
		&self,
		_kind: event::Kind,
		data: Handshake,
		connection: Connection,
		guarantee: Guarantee,
		local_data: &LocalData,
	) -> VoidResult {
		log::debug!("{}", data.display_name);
		if !local_data.is_local(&connection) {
			Network::send(
				Packet::builder()
					.with_address(connection.address)?
					.with_guarantee(guarantee)
					.with_payload(&data)
					.build(),
			)?;
		}
		Ok(())
	}
}

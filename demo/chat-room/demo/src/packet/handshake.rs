use crate::engine::{
	network::{
		connection::Connection,
		packet::{Guarantee, Packet, Processor},
		packet_kind, Network,
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
pub struct Handshake {}

impl Handshake {
	pub fn processor() -> Processor {
		use crate::engine::network::Kind::*;
		Processor::default()
			.with(Server, Self::send_back_to_client)
			.ignore(Client)
	}

	fn send_back_to_client(
		data: &mut Self,
		source: &Connection,
		guarantees: Guarantee,
	) -> VoidResult {
		Network::send(
			Packet::builder()
				.with_address(source.address)?
				.with_guarantee(guarantees)
				.with_payload(&*data)
				.build(),
		);
		Ok(())
	}
}

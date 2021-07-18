use crate::engine::{
	network::{
		self,
		packet::{AnyBox, Guarantee, Packet, Processor},
		packet_kind, Network,
	},
	utility::VoidResult,
};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;

#[packet_kind(crate::engine::network, "handshake", Handshake)]
#[derive(Serialize, Deserialize)]
pub struct Handshake {}

impl Processor<Handshake> for Handshake {
	fn process(data: &mut Self, source: SocketAddr, guarantees: Guarantee) -> VoidResult {
		if Network::read()
			.unwrap()
			.mode()
			.contains(network::Kind::Server)
		{
			Network::send(
				Packet::builder()
					.with_address(source)?
					.with_guarantee(guarantees)
					.with_payload(&*data)
					.build(),
			);
		}
		Ok(())
	}
}

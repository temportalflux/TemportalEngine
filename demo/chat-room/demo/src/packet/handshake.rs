use crate::engine::{
	network::{
		self,
		packet::{self, Guarantee, Packet, AnyBox, Processor},
		Network,
	},
	utility::{Registerable, VoidResult},
};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;

#[derive(Serialize, Deserialize)]
pub struct Handshake {}

impl Registerable<packet::KindId, packet::Registration> for Handshake {
	fn unique_id() -> packet::KindId {
		"handshake"
	}
	fn registration() -> packet::Registration
	where
		Self: Sized + 'static,
	{
		packet::Registration::of::<Self, Self>()
	}
}

impl packet::Kind for Handshake {
	fn serialize_to(&self) -> Vec<u8> {
		rmp_serde::to_vec(&self).unwrap()
	}

	fn deserialize_from(bytes: &[u8]) -> AnyBox
	where
		Self: Sized,
	{
		Box::new(rmp_serde::from_read_ref::<[u8], Handshake>(&bytes).unwrap())
	}
}

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

use crate::engine::{
	network::{
		packet::{AnyBox, Guarantee, Processor},
		packet_kind, LOG,
	},
	utility::VoidResult,
};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;

#[packet_kind(crate::engine::network, Message)]
#[derive(Serialize, Deserialize)]
pub struct Message {
	pub content: String,
}

impl Processor<Message> for Message {
	fn process(data: &mut Self, source: SocketAddr, _guarantees: Guarantee) -> VoidResult {
		log::debug!(target: LOG, "{} says: {}", source, data.content);
		Ok(())
	}
}

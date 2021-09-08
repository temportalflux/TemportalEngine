use crate::engine::{
	network::{
		connection::Connection,
		packet::{Guarantee, Packet, Processor},
		packet_kind, Network, LOG,
	},
	utility::VoidResult,
};
use serde::{Deserialize, Serialize};

#[packet_kind(crate::engine::network)]
#[derive(Serialize, Deserialize)]
pub struct Message {
	pub content: String,
	pub timestamp: chrono::DateTime<chrono::Utc>,
}

impl Message {
	pub fn new<T: Into<String>>(content: T) -> Self {
		Self {
			content: content.into(),
			timestamp: chrono::Utc::now(),
		}
	}

	pub fn processor() -> Processor {
		use crate::engine::network::Kind::*;
		// NOTE: Does not currently handle client-on-top-of-server
		Processor::default()
			.with(Server, Self::process_server)
			.with(Client, Self::process_client)
	}

	fn process_server(data: &mut Self, source: &Connection, guarantees: Guarantee) -> VoidResult {
		log::debug!(
			target: LOG,
			"{} said: \"{}\" at {}",
			source.id.unwrap(),
			data.content,
			data.timestamp.to_rfc2822()
		);

		//let sent_at = data.timestamp.clone();
		data.timestamp = chrono::Utc::now();

		Network::broadcast(
			Packet::builder()
				.with_guarantee(guarantees)
				.with_payload(&*data),
		)?;

		Ok(())
	}

	fn process_client(data: &mut Self, _source: &Connection, _guarantees: Guarantee) -> VoidResult {
		// NOTE: The source is always the server, so there is no current information about who actually sent the message
		log::debug!(
			target: LOG,
			"someone said: \"{}\" at {}",
			data.content,
			data.timestamp.to_rfc2822()
		);
		Ok(())
	}
}

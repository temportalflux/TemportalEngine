use crate::engine::{
	network::{
		self,
		connection::Connection,
		event, mode,
		packet::{Guarantee, Packet},
		packet_kind,
		processor::{EventProcessors, PacketProcessor, Processor},
		LocalData, Network, LOG,
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
	pub fn register(builder: &mut network::Builder) {
		use mode::Kind::*;
		builder.register_bundle::<Message>(
			EventProcessors::default()
				.with(Server, BroadcastMessage())
				.with(Client, SaveMessageToLog())
				.with(Server + Client, BroadcastAndSaveMessage()),
		);
	}

	pub fn new<T: Into<String>>(content: T) -> Self {
		Self {
			content: content.into(),
			timestamp: chrono::Utc::now(),
		}
	}
}

struct BroadcastMessage();

impl BroadcastMessage {
	fn rebroadcast(data: &mut Message, guarantee: &Guarantee) -> VoidResult {
		//let sent_at = data.timestamp.clone();
		data.timestamp = chrono::Utc::now();

		Network::broadcast(
			Packet::builder()
				.with_guarantee(*guarantee)
				.with_payload(data),
		)
	}
}

impl Processor for BroadcastMessage {
	fn process(
		&self,
		kind: event::Kind,
		data: Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Message> for BroadcastMessage {
	fn process_packet(
		&self,
		_kind: event::Kind,
		mut data: Message,
		connection: Connection,
		guarantee: Guarantee,
		_local_data: &LocalData,
	) -> VoidResult {
		log::debug!(
			target: LOG,
			"{} said: \"{}\" at {}",
			connection.id.unwrap(),
			data.content,
			data.timestamp.to_rfc2822()
		);
		BroadcastMessage::rebroadcast(&mut data, &guarantee)?;
		Ok(())
	}
}

struct SaveMessageToLog();

impl SaveMessageToLog {
	fn save_to_log(data: &Message) {
		if let Ok(mut history) = crate::MessageHistory::write() {
			history.add(crate::Message {
				content: data.content.clone(),
			});
		}
	}
}

impl Processor for SaveMessageToLog {
	fn process(
		&self,
		kind: event::Kind,
		data: Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Message> for SaveMessageToLog {
	fn process_packet(
		&self,
		_kind: event::Kind,
		data: Message,
		_connection: Connection,
		_guarantee: Guarantee,
		_local_data: &LocalData,
	) -> VoidResult {
		// NOTE: The source is always the server, so there is no current information about who actually sent the message
		log::debug!(
			target: LOG,
			"someone said: \"{}\" at {}",
			data.content,
			data.timestamp.to_rfc2822()
		);
		SaveMessageToLog::save_to_log(&data);
		Ok(())
	}
}

struct BroadcastAndSaveMessage();
impl Processor for BroadcastAndSaveMessage {
	fn process(
		&self,
		kind: event::Kind,
		data: Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		self.process_as(kind, data, local_data)
	}
}
impl PacketProcessor<Message> for BroadcastAndSaveMessage {
	fn process_packet(
		&self,
		_kind: event::Kind,
		mut data: Message,
		connection: Connection,
		guarantee: Guarantee,
		_local_data: &LocalData,
	) -> VoidResult {
		log::debug!(
			target: LOG,
			"{} said: \"{}\" at {}",
			connection.id.unwrap(),
			data.content,
			data.timestamp.to_rfc2822()
		);
		BroadcastMessage::rebroadcast(&mut data, &guarantee)?;
		SaveMessageToLog::save_to_log(&data);
		Ok(())
	}
}

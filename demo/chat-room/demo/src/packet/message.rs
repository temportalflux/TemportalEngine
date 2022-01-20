use crate::engine::{
	network::{
		self,
		connection::Connection,
		event, mode,
		packet::{Guarantee, Packet, PacketMode},
		packet_kind,
		processor::{EventProcessors, PacketProcessor, Processor},
		LocalData, Network, LOG,
	},
	utility::Result,
};
use serde::{Deserialize, Serialize};

#[packet_kind(crate::engine::network)]
#[derive(Serialize, Deserialize)]
pub struct Message {
	pub timestamp: chrono::DateTime<chrono::Utc>,
	pub sender_name: Option<String>,
	pub content: String,
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
			timestamp: chrono::Utc::now(),
			sender_name: None,
			content: content.into(),
		}
	}
}

struct BroadcastMessage();

impl BroadcastMessage {
	fn rebroadcast(
		data: &mut Message,
		connection: &Connection,
		guarantee: &Guarantee,
	) -> Result<()> {
		//let sent_at = data.timestamp.clone();
		data.timestamp = chrono::Utc::now();

		data.sender_name = None;
		if let Some(id) = &connection.id {
			if let Ok(history) = crate::MessageHistory::read() {
				if let Some(name) = history.get_user(id) {
					data.sender_name = Some(name.clone());
				}
			}
		}

		if data.sender_name.is_some() {
			Network::send_packets(
				Packet::builder()
					.with_mode(PacketMode::Broadcast)
					.ignore_local_address()
					.with_guarantee(*guarantee)
					.with_payload(data),
			)?;
		}

		Ok(())
	}
}

impl Processor for BroadcastMessage {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> Result<()> {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Message> for BroadcastMessage {
	fn process_packet(
		&self,
		_kind: &event::Kind,
		data: &mut Message,
		connection: &Connection,
		guarantee: &Guarantee,
		_local_data: &LocalData,
	) -> Result<()> {
		log::debug!(
			target: LOG,
			"{} said: \"{}\" at {}",
			connection.id.unwrap(),
			data.content,
			data.timestamp.to_rfc2822()
		);
		BroadcastMessage::rebroadcast(data, &connection, &guarantee)?;
		Ok(())
	}
}

pub struct SaveMessageToLog();

impl SaveMessageToLog {
	pub fn save_to_log(history: &mut crate::MessageHistory, data: &Message) {
		history.add(crate::Message {
			sender_name: data.sender_name.clone(),
			content: data.content.clone(),
		});
	}
}

impl Processor for SaveMessageToLog {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> Result<()> {
		self.process_as(kind, data, local_data)
	}
}

impl PacketProcessor<Message> for SaveMessageToLog {
	fn process_packet(
		&self,
		_kind: &event::Kind,
		data: &mut Message,
		_connection: &Connection,
		_guarantee: &Guarantee,
		_local_data: &LocalData,
	) -> Result<()> {
		// NOTE: The source is always the server, so there is no current information about who actually sent the message
		log::debug!(
			target: LOG,
			"someone said: \"{}\" at {}",
			data.content,
			data.timestamp.to_rfc2822()
		);
		if let Ok(mut history) = crate::MessageHistory::write() {
			SaveMessageToLog::save_to_log(&mut history, &data);
		}
		Ok(())
	}
}

struct BroadcastAndSaveMessage();
impl Processor for BroadcastAndSaveMessage {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> Result<()> {
		self.process_as(kind, data, local_data)
	}
}
impl PacketProcessor<Message> for BroadcastAndSaveMessage {
	fn process_packet(
		&self,
		_kind: &event::Kind,
		data: &mut Message,
		connection: &Connection,
		guarantee: &Guarantee,
		local_data: &LocalData,
	) -> Result<()> {
		log::debug!(
			target: LOG,
			"{} said: \"{}\" at {}",
			connection.id.unwrap(),
			data.content,
			data.timestamp.to_rfc2822()
		);
		if !local_data.is_local(&connection) {
			BroadcastMessage::rebroadcast(data, &connection, &guarantee)?;
		}
		if let Ok(mut history) = crate::MessageHistory::write() {
			SaveMessageToLog::save_to_log(&mut history, &data);
		}
		Ok(())
	}
}

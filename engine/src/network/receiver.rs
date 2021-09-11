use super::{connection, event, mode, packet, processor, LOG};
use crate::utility::{AnyError, VoidResult};
use std::sync::{Arc, RwLock};

pub struct Receiver {
	connection_list: Arc<RwLock<connection::List>>,
	queue: socknet::event::Queue,
	processor_registry: processor::Registry,
	type_registry: packet::Registry,
}

impl Receiver {
	fn deserialize_packet(&self, mut socknet_packet: packet::Packet) -> Option<packet::AnyBox> {
		let payload = socknet_packet.take_payload();
		return self
			.type_registry
			.types
			.get(socknet_packet.kind().as_str())
			.map(move |entry| entry.deserialize_from(payload.data()));
	}

	fn parse_event(
		&self,
		event: socknet::event::Event,
	) -> Result<(event::Kind, Option<event::Data>), AnyError> {
		use socknet::event::Event;

		let connection = match event.address() {
			Some(addr) => {
				if let Ok(list) = self.connection_list.read() {
					list.get_connection_by_addr(&addr)
				}
				else {
					None
				}
			}
			None => None,
		};
		let map_conn = |conn: Option<connection::Connection>| conn.map(|c| event::Data::Connection(c));

		return Ok(match event {
			Event::Connected(_) => (event::Kind::Connected, map_conn(connection)),
			Event::TimedOut(_) => (event::Kind::Timeout, map_conn(connection)),
			Event::Disconnected(_) => (event::Kind::Disconnected, map_conn(connection)),
			Event::Stop => (event::Kind::Stop, None),
			Event::Packet(packet) => {
				let guarantee = *packet.guarantees();
				let packet_kind = packet.kind().clone();
				let any_packet = self.deserialize_packet(packet);
				(
					event::Kind::Packet(packet_kind),
					connection
						.zip(any_packet)
						.map(|(conn, parsed_packet)| event::Data::Packet(conn, guarantee, parsed_packet)),
				)
			}
		});
	}

	pub fn process(&self, mode: &mode::Set) -> VoidResult {
		loop {
			match self.queue.channel().try_recv() {
				Ok(event) => {
					let (event_kind, event_data) = self.parse_event(event)?;

					match (&event_kind, &event_data) {
						(event::Kind::Packet(packet_kind), None) => {
							log::error!(
								target: LOG,
								"Failed to parse packet with kind({})",
								packet_kind
							);
							continue;
						}
						(_, _) => {}
					}

					// The first option indicates if the processor has a configuration for the net mode
					let opt_processor = match self
						.processor_registry
						.types
						.get(&event_kind)
						.map(|processor| processor.get_for_mode(&mode))
						.flatten()
					{
						Some(processor) => processor,
						None => {
							log::warn!(
								target: LOG,
								"Ignoring event {} on net mode {}, no processor found.",
								event_kind,
								mode.iter()
									.map(|kind| kind.to_string())
									.collect::<Vec<_>>()
									.join("+")
							);
							continue;
						}
					};

					// the second option indicates if the processor is explicitly ignoring the mode or not
					if let Some(processor) = opt_processor {
						if let Err(err) = processor.process(event_kind, event_data) {
							log::error!(target: LOG, "{}", err);
						}
					}
				}
				Err(socknet::channel::TryRecvError::Empty) => break,
				Err(socknet::channel::TryRecvError::Disconnected) => break,
			}
		}
		Ok(())
	}
}

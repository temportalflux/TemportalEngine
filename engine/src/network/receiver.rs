use super::{connection, event, mode, packet, processor, LOG};
use crate::utility::{AnyError, VoidResult};
use std::sync::{Arc, Mutex, RwLock};

pub struct Receiver {
	pub(super) connection_list: Arc<RwLock<connection::List>>,
	pub(super) queue: socknet::event::Queue,
	pub(super) processor_registry: Arc<Mutex<processor::Registry>>,
	pub(super) type_registry: Arc<Mutex<packet::Registry>>,
}

impl Receiver {
	fn deserialize_packet(&self, mut socknet_packet: packet::Packet) -> Option<packet::AnyBox> {
		let payload = socknet_packet.take_payload();
		match self.type_registry.lock() {
			Ok(reg_guard) => (*reg_guard)
				.types
				.get(socknet_packet.kind().as_str())
				.map(move |entry| entry.deserialize_from(payload.data())),
			Err(_) => None,
		}
	}

	fn parse_event(
		&self,
		event: socknet::event::Event,
	) -> Result<(event::Kind, Option<event::Data>), AnyError> {
		use socknet::event::Event;

		let make_addr_datum = |address| Some(event::Data::Address(address));
		return Ok(match event {
			Event::Connected(addr) => (event::Kind::Connected, make_addr_datum(addr)),
			Event::TimedOut(addr) => (event::Kind::Timeout, make_addr_datum(addr)),
			Event::Disconnected(addr) => (event::Kind::Disconnected, make_addr_datum(addr)),
			Event::Stop => (event::Kind::Stop, None),
			Event::Packet(packet) => {
				let connection = if let Ok(list) = self.connection_list.read() {
					list.get_connection_by_addr(packet.address())
				} else {
					None
				};
				let guarantee = *packet.guarantees();
				let packet_kind = packet.kind().clone();
				let any_packet = self.deserialize_packet(packet);
				(
					event::Kind::Packet(packet_kind),
					connection.zip(any_packet).map(|(conn, parsed_packet)| {
						event::Data::Packet(conn, guarantee, parsed_packet)
					}),
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
					let reg_guard = match self.processor_registry.lock() {
						Ok(guard) => guard,
						Err(_) => continue,
					};
					let opt_processor = (*reg_guard)
						.types
						.get(&event_kind)
						.map(|processor| processor.get_for_mode(&mode))
						.flatten();
					let opt_processor = match opt_processor {
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

impl Drop for Receiver {
	fn drop(&mut self) {
		log::debug!(target: LOG, "Dropping network receiver");
	}
}

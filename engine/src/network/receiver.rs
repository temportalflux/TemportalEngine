use super::{
	connection, packet, processor, mode, LOG, event
};
use crate::utility::VoidResult;
use std::{
	sync::{Arc, RwLock, Weak},
};

pub struct Receiver {
	connection_list: Weak<RwLock<connection::List>>,
	queue: socknet::event::Queue,
	processor_registry: processor::Registry,
	type_registry: packet::Registry,
}

impl Receiver {

	fn deserialize_packet(&self, socknet_packet: packet::Packet) -> Option<packet::AnyBox> {
		let payload = socknet_packet.take_payload();
		return self.type_registry.types.get(socknet_packet.kind().as_str()).map(move |entry| {
			entry.deserialize_from(payload.data())
		});
	}

	fn get_processor_for(&self, event_key: &event::Kind, mode: &mode::Set) -> Option<Option<packet::FnProcessAny>> {
		return self.processor_registry.types.get(event_key).map(|processor| processor.get_for_mode(&mode)).flatten();
	}

	fn parse_event(&self, event: socknet::event::Event) -> (event::Kind, Option<event::Data>)
	{
		use socknet::event::Event;
		return match event {
			Event::Connected(address) => (event::Kind::Connected, Some(event::Data::Address(address))),
			Event::TimedOut(address) => (event::Kind::Timeout, Some(event::Data::Address(address))),
			Event::Disconnected(address) => (event::Kind::Disconnected, Some(event::Data::Address(address))),
			Event::Stop => (event::Kind::Stop, None),
			Event::Packet(mut packet) => {
				let address = packet.address();
				let guarantee = packet.guarantees();
				let packet_kind = packet.kind();
				let any_packet = self.deserialize_packet(packet);
				(event::Kind::Packet(packet_kind.clone()), any_packet.map(|packet| event::Data::Packet(*address, *guarantee, packet)))
			}
		};
	}
	
	pub fn process(&mut self, mode: &mode::Set) -> VoidResult {
		loop {
			match self.queue.channel().try_recv() {
				Ok(event) => {
					let (event_kind, event_data) = self.parse_event(event);

					match (event_kind, event_data) {
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
					let processor = match self.get_processor_for(&event_kind, &mode) {
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
					if let Some(process_fn) = processor {
						(process_fn)(event_data, &connection)
					}
				}
				Err(socknet::TryRecvError::Empty) => break,
				Err(socknet::TryRecvError::Disconnected) => break,
			}
		}
		Ok(())
	}

}

use super::{
	connection::{self, Connection},
	event, packet, processor, LocalData, LOG,
};
use crate::utility::{AnyError, VoidResult};
use std::sync::{atomic, Arc, Mutex, RwLock};

pub struct Receiver {
	pub(super) connection_list: Arc<RwLock<connection::List>>,
	pub(super) queue: socknet::event::Queue,
	pub(super) flag_should_be_destroyed: Arc<atomic::AtomicBool>,
	pub(super) processor_registry: Arc<Mutex<processor::Registry>>,
	pub(super) type_registry: Arc<Mutex<packet::Registry>>,
	pub(super) local_data: LocalData,
}

impl Receiver {
	pub fn should_be_destroyed(&self) -> bool {
		self.flag_should_be_destroyed
			.load(atomic::Ordering::Relaxed)
	}

	#[profiling::function]
	fn deserialize_packet(&self, mut socknet_packet: packet::Packet) -> Option<packet::AnyBox> {
		let payload = socknet_packet.take_payload();
		if let Ok(guard) = self.type_registry.lock() {
			if let Some(registration) = (*guard).types.get(socknet_packet.kind().as_str()) {
				return Some(registration.deserialize_from(payload.data()));
			} else {
				log::warn!(
					target: LOG,
					"Failed to deserialize packet with id \"{}\", not registered.",
					socknet_packet.kind()
				);
			}
		}
		None
	}

	fn get_connection(&self, address: std::net::SocketAddr) -> Connection {
		if let Ok(list) = self.connection_list.read() {
			if let Some(conn) = list.get_connection_by_addr(&address) {
				return conn;
			}
		}
		// There was no active connection, so this must be the first packet received by the client.
		// Its ok that there is no id, and processors of the packet
		// should know what their first packet is and how to handle it.
		Connection { id: None, address }
	}

	#[profiling::function]
	fn parse_event(
		&self,
		event: socknet::event::Event,
	) -> Result<(event::Kind, Option<event::Data>), AnyError> {
		use socknet::event::Event;

		let make_addr_datum = |address| Some(event::Data::Connection(self.get_connection(address)));
		return Ok(match event {
			Event::Connected(addr) => (event::Kind::Connected, make_addr_datum(addr)),
			Event::TimedOut(addr) => (event::Kind::Timeout, make_addr_datum(addr)),
			Event::Disconnected(addr) => (event::Kind::Disconnected, make_addr_datum(addr)),
			Event::Stop => (event::Kind::Stop, None),
			Event::Packet(packet) => {
				let connection = self.get_connection(*packet.address());
				//log::debug!(target: LOG, "Received packet {:?}", packet);
				let guarantee = *packet.guarantees();
				let packet_kind = packet.kind().clone();
				let any_packet = self.deserialize_packet(packet);
				(
					event::Kind::Packet(packet_kind),
					any_packet.map(|parsed_packet| {
						event::Data::Packet(connection, guarantee, parsed_packet)
					}),
				)
			}
		});
	}

	#[profiling::function]
	pub fn process(&self) -> VoidResult {
		loop {
			profiling::scope!("receive-event");
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
					self.process_event(event_kind, event_data);
				}
				Err(socknet::channel::TryRecvError::Empty) => break,
				Err(socknet::channel::TryRecvError::Disconnected) => break,
			}
		}
		Ok(())
	}

	#[profiling::function]
	fn process_event(&self, event_kind: event::Kind, mut event_data: Option<event::Data>) {
		let reg_guard = match self.processor_registry.lock() {
			Ok(guard) => guard,
			Err(_) => return,
		};
		let opt_processors = (*reg_guard)
			.types
			.get(&event_kind)
			.map(|processor| processor.get_for_mode(&self.local_data.mode))
			.flatten();
		let processors = match opt_processors {
			Some(processors) => processors,
			None => {
				log::warn!(
					target: LOG,
					"Ignoring event {} on net mode {}, no processor found.",
					event_kind,
					self.local_data
						.mode
						.iter()
						.map(|kind| kind.to_string())
						.collect::<Vec<_>>()
						.join("+")
				);
				return;
			}
		};

		for processor in processors.iter() {
			if let Err(err) =
				processor.process(&event_kind, &mut event_data, &self.local_data)
			{
				log::error!(target: LOG, "{}", err);
			}
		}
	}
}

impl Drop for Receiver {
	fn drop(&mut self) {
		log::debug!(target: LOG, "Dropping network receiver");
	}
}

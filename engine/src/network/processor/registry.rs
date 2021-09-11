use super::super::{connection::Connection, event, mode, packet};
use crate::utility::VoidResult;
use std::collections::HashMap;
use temportal_engine_utilities::registry::Registry as GenericRegistry;

pub type Registry = GenericRegistry<event::Kind, EventProcessors>;

/// Saves information about how a packet is handled/processed,
/// so the proper function can be executed when the packet is received.
pub struct EventProcessors {
	processor_by_mode: HashMap<mode::Set, Option<Box<dyn Processor + 'static>>>,
}

impl Default for EventProcessors {
	fn default() -> Self {
		Self {
			processor_by_mode: HashMap::new(),
		}
	}
}

impl EventProcessors {
	/// Marks that the packet is ignored for a given net mode.
	///
	/// This is helpful for packets such as a handshake,
	/// which need to be sent between both ends of a connection,
	/// but one of the receivers doesn't need to act on it.
	///
	/// The net mode is a collection of flags, all of which must be present for the packet to be ignored.
	/// Example: Specifing `Client + Server` as the net mode says that the packet is ignored only
	/// if received on a Client-On-Top-Of-Server. It does NOT mean that the packet is ignored
	/// for both Dedicated Clients AND Dedicated Servers. That can be achieved by using `.ignore(Client).ignore(Server)`.
	pub fn ignore<TNetMode>(mut self, net_mode: TNetMode) -> Self
	where
		TNetMode: Into<mode::Set>,
	{
		self.processor_by_mode.insert(net_mode.into(), None);
		self
	}

	pub fn with<TNetMode, TProc>(mut self, net_mode: TNetMode, processor: TProc) -> Self
	where
		TNetMode: Into<mode::Set>,
		TProc: Processor + 'static,
	{
		self.insert(net_mode, processor);
		self
	}

	pub fn insert<TNetMode, TProc>(&mut self, net_mode: TNetMode, processor: TProc)
	where
		TNetMode: Into<mode::Set>,
		TProc: Processor + 'static,
	{
		self.processor_by_mode
			.insert(net_mode.into(), Some(Box::new(processor)));
	}

	pub fn insert_box<TNetMode>(
		&mut self,
		net_mode: TNetMode,
		processor: Box<dyn Processor + 'static>,
	) where
		TNetMode: Into<mode::Set>,
	{
		self.processor_by_mode
			.insert(net_mode.into(), Some(processor));
	}

	/// Returns the callback to be used to process the packet based on the provided net mode.
	///
	/// This function returns a double optional. The first indicates if there was any configuration
	/// provided for the net mode (either a valid callback OR marking the packet as ignored).
	/// The second optional provides the actual callback if the packet is not marked as ignored.
	pub fn get_for_mode(
		&self,
		net_mode: &mode::Set,
	) -> Option<&Option<Box<dyn Processor + 'static>>> {
		self.processor_by_mode.get(net_mode)
	}
}

/// Processes a single [`event`](crate::network::event::Kind) for a given [`net mode`](crate::network::mode::Set).
pub trait Processor {
	fn process(&self, kind: event::Kind, data: Option<event::Data>) -> VoidResult;
}

/// Helper class which interprets an event as a type of packet,
/// automatically downcasting to the indicated type.
pub trait PacketProcessor<TPacketKind: 'static>: Processor {
	fn process(&self, kind: event::Kind, data: Option<event::Data>) -> VoidResult {
		if let Some(event::Data::Packet(source, guarantee, boxed)) = data {
			return self.process_packet(
				kind,
				*boxed.downcast::<TPacketKind>().unwrap(),
				source,
				guarantee,
			);
		} else {
			Err(Box::new(Error::EncounteredNonPacket(kind)))
		}
	}

	fn process_packet(
		&self,
		kind: event::Kind,
		data: TPacketKind,
		connection: Connection,
		guarantee: packet::Guarantee,
	) -> VoidResult;
}

#[derive(Debug)]
enum Error {
	EncounteredNonPacket(event::Kind),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match self {
			Error::EncounteredNonPacket(ref kind) => write!(
				f,
				"Packet-Only processor cannot handle non-packet event: {}",
				kind
			),
		}
	}
}

impl std::error::Error for Error {}

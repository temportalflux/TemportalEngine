use super::super::{connection::Connection, event, mode, packet, LocalData};
use crate::utility::VoidResult;
use std::collections::HashMap;
use temportal_engine_utilities::registry::Registry as GenericRegistry;

pub type Registry = GenericRegistry<event::Kind, EventProcessors>;

/// Saves information about how a packet is handled/processed,
/// so the proper function can be executed when the packet is received.
pub struct EventProcessors {
	procs_by_mode: HashMap<mode::Set, Vec<Box<dyn Processor + 'static>>>,
}

impl Default for EventProcessors {
	fn default() -> Self {
		Self {
			procs_by_mode: HashMap::new(),
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
		self.procs_by_mode.insert(net_mode.into(), vec![]);
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
		self.insert_box(net_mode, Box::new(processor));
	}

	pub fn insert_box<TNetMode>(
		&mut self,
		net_mode: TNetMode,
		processor: Box<dyn Processor + 'static>,
	) where
		TNetMode: Into<mode::Set>,
	{
		let mode = net_mode.into();
		if !self.procs_by_mode.contains_key(&mode) {
			self.procs_by_mode.insert(mode.clone(), vec![]);
		}
		self.procs_by_mode.get_mut(&mode).unwrap().push(processor);
	}

	/// Returns the callback to be used to process the packet based on the provided net mode.
	///
	/// This function returns an optional-vec. The first indicates if there was any configuration
	/// provided for the net mode (either a valid callback OR marking the packet as ignored).
	/// The vec provides the actual callbacks if the packet is not marked as ignored.
	pub fn get_for_mode(&self, net_mode: &mode::Set) -> Option<&Vec<Box<dyn Processor + 'static>>> {
		self.procs_by_mode.get(net_mode)
	}
}

/// Processes a single [`event`](crate::network::event::Kind) for a given [`net mode`](crate::network::mode::Set).
pub trait Processor {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult;

	fn boxed(self) -> Box<dyn Processor + 'static> where Self: 'static + Sized {
		Box::new(self)
	}

}

pub struct AnyProcessor(Vec<Box<dyn Processor + 'static>>);
impl AnyProcessor {
	pub fn new(any_of: Vec<Box<dyn Processor + 'static>>) -> Self {
		Self(any_of)
	}
}
impl Processor for AnyProcessor {
	fn process(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		let mut result = Ok(()); // should having no processors be a warning?
		for processor in self.0.iter() {
			result = processor.process(kind, data, local_data);
			if result.is_ok() {
				return result;
			}
		}
		result
	}
}

/// Helper class which interprets an event as a type of packet,
/// automatically downcasting to the indicated type.
pub trait PacketProcessor<TPacketKind: 'static>: Processor + 'static {
	fn process_as(
		&self,
		kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		if let Some(event::Data::Packet(source, guarantee, boxed)) = data {
			self.process_packet(
				kind,
				&mut *boxed.downcast_mut::<TPacketKind>().unwrap(),
				source,
				guarantee,
				local_data,
			)
		} else {
			Err(Box::new(super::super::Error::EncounteredNonPacket(
				kind.clone(),
			)))
		}
	}

	fn process_packet(
		&self,
		kind: &event::Kind,
		data: &mut TPacketKind,
		connection: &Connection,
		guarantee: &packet::Guarantee,
		local_data: &LocalData,
	) -> VoidResult;
}

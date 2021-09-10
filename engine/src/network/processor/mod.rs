use super::{connection::Connection, packet, mode, event};
use crate::utility::VoidResult;
use std::{collections::HashMap, sync::Arc};
use temportal_engine_utilities::registry::Registry as GenericRegistry;

pub type FnProcessEvent = Arc<Box<dyn Fn(event::Data) -> VoidResult>>;
pub type Registry = GenericRegistry<event::Kind, Processor>;

/// Saves information about how a packet is handled/processed,
/// so the proper function can be executed when the packet is received.
pub struct Processor {
	process_functions: HashMap<mode::Set, Option<FnProcessEvent>>,
}

impl Default for Processor {
	fn default() -> Self {
		Self {
			process_functions: HashMap::new(),
		}
	}
}

impl Processor {
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
		self.process_functions.insert(net_mode.into(), None);
		self
	}

	/// Associates a net mode with a specific callback,
	/// allowing packets to have different callbacks
	/// based on what net mode the receiver is on.
	pub fn with_packet<TPacketKind, TNetMode, TProcessCallback>(
		mut self,
		net_mode: TNetMode,
		process_fn: TProcessCallback,
	) -> Self
	where
		TNetMode: Into<mode::Set>,
		TPacketKind: packet::Kind + 'static,
		TProcessCallback: Fn(&mut TPacketKind, &Connection, packet::Guarantee) -> VoidResult + 'static,
	{
		let boxed_process_fn = Arc::new(Box::new(process_fn));
		self.process_functions.insert(
			net_mode.into(),
			Some(Arc::new(Box::new(
				move |event_data: event::Data| -> VoidResult {
					if let event::Data::Packet(source, guarantee, boxed) = event_data {
						// boxed: packet::AnyBox, source: &Connection, guarantee: packet::Guarantee
						(*boxed_process_fn)(
							&mut *boxed.downcast::<TPacketKind>().unwrap(),
							source,
							guarantee,
						)
					}
					else
					{
						Err(Box::new("Processor cannot handle non-packet event"))
					}
				},
			))),
		);
		self
	}

	/// Returns the callback to be used to process the packet based on the provided net mode.
	///
	/// This function returns a double optional. The first indicates if there was any configuration
	/// provided for the net mode (either a valid callback OR marking the packet as ignored).
	/// The second optional provides the actual callback if the packet is not marked as ignored.
	pub fn get_for_mode(&self, net_mode: &mode::Set) -> Option<Option<FnProcessAny>> {
		self.process_functions.get(net_mode).map(|v| v.clone())
	}
}

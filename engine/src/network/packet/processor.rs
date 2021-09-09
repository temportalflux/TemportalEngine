use super::{super::connection::Connection, Guarantee, Kind};
use crate::network;
use crate::utility::VoidResult;
use socknet::packet::AnyBox;
use std::collections::HashMap;
use std::sync::Arc;

pub type FnProcessAny = Arc<Box<dyn Fn(AnyBox, &Connection, Guarantee) -> VoidResult>>;

/// Saves information about how a packet is handled/processed,
/// so the proper function can be executed when the packet is received.
pub struct Processor {
	process_functions: HashMap<network::KindSet, Option<FnProcessAny>>,
}
pub(super) type ProcessorRegistry =
	temportal_engine_utilities::registry::Registry<socknet::packet::KindId, Processor>;

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
		TNetMode: Into<network::KindSet>,
	{
		self.process_functions.insert(net_mode.into(), None);
		self
	}

	/// Associates a net mode with a specific callback,
	/// allowing packets to have different callbacks
	/// based on what net mode the receiver is on.
	pub fn with<TPacketKind, TNetMode, TProcessCallback>(
		mut self,
		net_mode: TNetMode,
		process_fn: TProcessCallback,
	) -> Self
	where
		TNetMode: Into<network::KindSet>,
		TPacketKind: Kind + 'static,
		TProcessCallback: Fn(&mut TPacketKind, &Connection, Guarantee) -> VoidResult + 'static,
	{
		let boxed_process_fn = Arc::new(Box::new(process_fn));
		self.process_functions.insert(
			net_mode.into(),
			Some(Arc::new(Box::new(
				move |boxed: AnyBox, source: &Connection, guarantee: Guarantee| -> VoidResult {
					(*boxed_process_fn)(
						&mut *boxed.downcast::<TPacketKind>().unwrap(),
						source,
						guarantee,
					)
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
	pub fn get_for_mode(&self, net_mode: &network::KindSet) -> Option<Option<FnProcessAny>> {
		self.process_functions.get(net_mode).map(|v| v.clone())
	}
}

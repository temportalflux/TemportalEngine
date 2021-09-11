use super::{
	super::{event, LOG},
	Processor,
};
use crate::utility::VoidResult;
use std::sync::{atomic::{AtomicBool, Ordering}, Arc};

pub struct EndNetwork(Arc<AtomicBool>);

impl EndNetwork {
	pub fn new(flag: Arc<AtomicBool>) -> Self {
		Self(flag)
	}

	pub fn should_be_destroyed(&self) -> &Arc<AtomicBool> {
		&self.0
	}
}

impl Processor for EndNetwork {
	fn process(&self, kind: event::Kind, _data: Option<event::Data>) -> VoidResult {
		if let event::Kind::Stop = kind {
			log::debug!(
				target: LOG,
				"Encountered Stop event, marking network to be destroyed"
			);
			self.0.store(true, Ordering::Relaxed);
		}
		Ok(())
	}
}

use super::{AnyBox, Guarantee, Kind, Processor};
use crate::VoidResult;
use std::{net::SocketAddr, sync::Arc};
use temportal_engine_utilities::registry;

pub type KindId = &'static str;
pub type FnProcessKind = Arc<Box<dyn Fn(AnyBox, SocketAddr, Guarantee) -> VoidResult>>;
pub type Registry = registry::Registry<KindId, Registration>;

pub struct Registration {
	deserialize: Box<dyn Fn(&[u8]) -> AnyBox>,
	process: FnProcessKind,
}

impl Registration {
	pub fn of<T: Kind + 'static, TProc: Processor<T> + 'static>() -> Self {
		Self {
			deserialize: Box::new(T::deserialize_from),
			process: Arc::new(Box::new(TProc::process_boxed)),
		}
	}

	pub fn deserialize_from(&self, bytes: &[u8]) -> AnyBox {
		(self.deserialize)(bytes)
	}

	pub fn process_fn(&self) -> FnProcessKind {
		self.process.clone()
	}
}

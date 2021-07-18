use super::Guarantee;
use crate::utility::{Registerable, VoidResult};
use std::{net::SocketAddr, sync::Arc, any::Any};

pub type KindId = &'static str;
pub type KindIdOwned = String;
pub type AnyBox = Box<dyn Any + 'static + Send>;

pub trait Kind: Registerable<KindId, Registration> {
	fn serialize_to(&self) -> Vec<u8>;

	fn deserialize_from(bytes: &[u8]) -> AnyBox
	where
		Self: Sized;
}

pub type FnProcessKind = Arc<Box<dyn Fn(AnyBox, SocketAddr, Guarantee) -> VoidResult>>;

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

pub trait Processor<TKind> where TKind: Kind + Sized + 'static {
	fn process_boxed(boxed: AnyBox, source: SocketAddr, guarantee: Guarantee) -> VoidResult {
		Self::process(&mut *boxed.downcast::<TKind>().unwrap(), source, guarantee)
	}

	fn process(data: &mut TKind, source: SocketAddr, guarantee: Guarantee) -> VoidResult;
}

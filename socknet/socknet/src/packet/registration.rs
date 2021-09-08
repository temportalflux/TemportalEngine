use super::{AnyBox, Kind};
use temportal_engine_utilities::registry;

pub type KindId = &'static str;
pub type Registry = registry::Registry<KindId, Registration>;

pub struct Registration {
	deserialize: Box<dyn Fn(&[u8]) -> AnyBox>,
}

impl Registration {
	pub fn of<T: Kind + 'static>() -> Self {
		Self {
			deserialize: Box::new(T::deserialize_from),
		}
	}

	pub fn deserialize_from(&self, bytes: &[u8]) -> AnyBox {
		(self.deserialize)(bytes)
	}
}

use crate::utility::Registerable;

pub type KindId = &'static str;
pub type KindIdOwned = String;

pub trait Kind: Registerable<KindId, Registration> {
	fn registration() -> Registration
	where
		Self: Sized + 'static,
	{
		Registration::of::<Self>()
	}

	fn serialize_to(&self) -> Vec<u8>;

	fn deserialize_from(bytes: &[u8]) -> Box<dyn Kind>
	where
		Self: Sized;
}

pub struct Registration {
	deserialize: Box<dyn Fn(&[u8]) -> Box<dyn Kind>>,
}

impl Registration {
	pub fn of<T: Kind + 'static>() -> Self {
		Self {
			deserialize: Box::new(T::deserialize_from),
		}
	}

	pub fn deserialize_from(&self, bytes: &[u8]) -> Box<dyn Kind> {
		(self.deserialize)(bytes)
	}
}

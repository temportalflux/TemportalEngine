use super::{KindId, Registration};

pub type AnyBox = Box<dyn std::any::Any + 'static + Send>;

pub trait Kind: super::Registerable<KindId, Registration> {
	fn serialize_to(&self) -> Vec<u8>;

	fn deserialize_from(bytes: &[u8]) -> AnyBox
	where
		Self: Sized;
}

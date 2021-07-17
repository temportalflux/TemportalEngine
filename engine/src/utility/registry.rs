use std::collections::HashMap;

pub struct Registry<TKey, TRegistration>
where
	TKey: std::hash::Hash + std::cmp::Eq,
{
	pub types: HashMap<TKey, TRegistration>,
}

pub trait Registerable<TKey, TRegistration>
where
	TKey: std::hash::Hash + std::cmp::Eq,
{
	fn unique_id() -> TKey
	where
		Self: Sized;

	fn registration() -> TRegistration
	where
		Self: Sized;
}

impl<TKey, TRegistration> Registry<TKey, TRegistration>
where
	TKey: std::hash::Hash + std::cmp::Eq,
{
	pub fn new() -> Self {
		Self {
			types: HashMap::new(),
		}
	}

	pub fn register<T>(&mut self)
	where
		T: Registerable<TKey, TRegistration> + 'static,
	{
		self.types.insert(T::unique_id(), T::registration());
	}
}

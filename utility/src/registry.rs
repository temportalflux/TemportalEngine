use std::collections::HashMap;

pub struct Registry<TKey, TRegistration>
where
	TKey: std::hash::Hash + std::cmp::Eq,
{
	pub types: HashMap<TKey, TRegistration>,
}

impl<TKey, TRegistration> Clone for Registry<TKey, TRegistration>
where
	TKey: std::hash::Hash + std::cmp::Eq + Clone,
	TRegistration: Clone,
{
	fn clone(&self) -> Self {
		Self {
			types: self.types.clone(),
		}
	}
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

	pub fn insert(&mut self, key: TKey, value: TRegistration) {
		self.types.insert(key, value);
	}

	pub fn contains(&self, key: &TKey) -> bool {
		self.types.contains_key(key)
	}

	pub fn get_mut(&mut self, key: &TKey) -> Option<&mut TRegistration> {
		self.types.get_mut(key)
	}
}

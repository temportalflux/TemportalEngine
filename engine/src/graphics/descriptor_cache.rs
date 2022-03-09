use crate::graphics::descriptor::{self, layout::SetLayout};
use std::{
	collections::HashMap,
	sync::{self, Arc, RwLock},
};

/// An engine-level abstraction to handle creating/managing [`Descriptor Sets`](descriptor::Set)
/// which all have the same [`Layout`](SetLayout).
///
/// Users provide the key-type `T` which are used to identify different sets.
/// Users are also responsible for writing to each descriptor set that they create via [`insert`](DescriptorCache::insert).
pub struct DescriptorCache<T: Eq + std::hash::Hash> {
	sets: HashMap<T, sync::Weak<descriptor::Set>>,
	layout: sync::Arc<SetLayout>,
}

impl<T> DescriptorCache<T>
where
	T: Eq + std::hash::Hash,
{
	/// Creates a descriptor cache with a provided [`Layout`](SetLayout).
	pub fn new(layout: SetLayout) -> Self {
		Self {
			layout: sync::Arc::new(layout),
			sets: HashMap::new(),
		}
	}

	/// Returns a thread-safe reference-counted pointer to the descriptor layout.
	pub fn layout(&self) -> &sync::Arc<SetLayout> {
		&self.layout
	}

	/// Allocates a new descriptor set using the provided layout from the
	/// [`Chain`](crate::graphics::Chain)'s [`persistant descriptor pool`](crate::graphics::Chain::persistent_descriptor_pool),
	/// inserting the set into the cache and returning the thread-safe weak pointer.
	pub fn insert(
		&mut self,
		id: T,
		name: Option<String>,
		descriptor_pool: &Arc<RwLock<descriptor::Pool>>,
	) -> anyhow::Result<sync::Weak<descriptor::Set>> {
		let descriptor_set = descriptor_pool
			.write()
			.unwrap()
			.allocate_named_descriptor_sets(&vec![(
				self.layout().clone(),
				name.map(|v| format!("{}.Descriptor", v)),
			)])?
			.pop()
			.unwrap();
		self.sets.insert(id, descriptor_set.clone());
		Ok(descriptor_set)
	}

	/// Returns true if the cache contains a set with the provided id.
	/// If the id exists, the set can be obtained by using the index operator [`cache[id]`](std::ops::Index::index).
	pub fn contains(&self, id: &T) -> bool {
		self.sets.contains_key(id)
	}
}

impl<T> std::ops::Index<&T> for DescriptorCache<T>
where
	T: Eq + std::hash::Hash,
{
	type Output = sync::Weak<descriptor::Set>;
	fn index(&self, id: &T) -> &Self::Output {
		&self.sets[id]
	}
}

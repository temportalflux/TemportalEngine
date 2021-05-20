use crate::{
	graphics::{descriptor, RenderChain},
	utility,
};
use std::{collections::HashMap, sync};

pub struct DescriptorCache {
	sets: HashMap<String, sync::Weak<descriptor::Set>>,
	layout: sync::Arc<descriptor::SetLayout>,
}

impl DescriptorCache {
	pub fn new(layout: descriptor::SetLayout) -> Self {
		Self {
			layout: sync::Arc::new(layout),
			sets: HashMap::new(),
		}
	}

	pub fn layout(&self) -> &sync::Arc<descriptor::SetLayout> {
		&self.layout
	}

	pub fn insert(
		&mut self,
		id: String,
		render_chain: &RenderChain,
	) -> utility::Result<sync::Weak<descriptor::Set>> {
		let descriptor_set = render_chain
			.persistent_descriptor_pool()
			.write()
			.unwrap()
			.allocate_descriptor_sets(&vec![self.layout().clone()])?
			.pop()
			.unwrap();
		self.sets.insert(id, descriptor_set.clone());
		Ok(descriptor_set)
	}

	pub fn contains(&self, id: &String) -> bool {
		self.sets.contains_key(id)
	}
}

impl std::ops::Index<&String> for DescriptorCache {
	type Output = sync::Weak<descriptor::Set>;
	fn index(&self, id: &String) -> &Self::Output {
		&self.sets[id]
	}
}

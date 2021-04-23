use crate::asset;
use std::collections::HashMap;

/// Runtime listings of the kinds of assets that exist.
/// Each asset type corresponds one-to-one with an implementation of the [`Asset`](crate::asset::Asset) trait.
pub struct TypeRegistry {
	types: HashMap<asset::TypeId, Box<dyn asset::TypeMetadata>>,
}

impl TypeRegistry {
	pub fn new() -> TypeRegistry {
		TypeRegistry {
			types: HashMap::new(),
		}
	}

	pub fn register<T>(&mut self)
	where
		T: asset::Asset,
	{
		let metadata = T::metadata();
		if !self.types.contains_key(metadata.name()) {
			log::info!(target: asset::LOG, "Registring asset type \"{}\"", metadata.name());
			self.types.insert(metadata.name(), metadata);
		}
		else
		{
			log::error!(target: asset::LOG, "Encountered duplicate asset type \"{}\"", metadata.name());
		}
	}

	pub fn get(&self, type_id: &str) -> Option<&Box<dyn asset::TypeMetadata>> {
		self.types.get(type_id)
	}
}

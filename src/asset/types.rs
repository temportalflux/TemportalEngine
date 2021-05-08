use crate::asset;
use std::collections::HashMap;
use std::sync::RwLock;

/// Runtime listings of the kinds of assets that exist.
/// Each asset type corresponds one-to-one with an implementation of the [`Asset`](crate::asset::Asset) trait.
pub struct TypeRegistry {
	types: HashMap<asset::TypeId, Box<dyn asset::TypeMetadata>>,
}

pub type RegisteredType<'a> = Option<&'a Box<dyn asset::TypeMetadata>>;

impl Default for TypeRegistry {
	fn default() -> TypeRegistry {
		TypeRegistry {
			types: HashMap::new(),
		}
	}
}

impl TypeRegistry {
	pub fn get() -> &'static RwLock<TypeRegistry> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<TypeRegistry> = Singleton::uninit();
		unsafe { INSTANCE.get() }
	}

	pub fn read() -> std::sync::RwLockReadGuard<'static, TypeRegistry> {
		TypeRegistry::get().read().unwrap()
	}
}

impl TypeRegistry {
	pub fn register<T>(&mut self)
	where
		T: asset::Asset,
	{
		let metadata = T::metadata();
		if !self.types.contains_key(metadata.name()) {
			log::info!(
				target: asset::LOG,
				"Registring asset type \"{}\"",
				metadata.name()
			);
			self.types.insert(metadata.name(), metadata);
		} else {
			log::error!(
				target: asset::LOG,
				"Encountered duplicate asset type \"{}\"",
				metadata.name()
			);
		}
	}

	pub fn at<'a>(&'a self, type_id: &str) -> RegisteredType<'a> {
		self.types.get(type_id)
	}
}

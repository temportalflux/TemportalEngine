use crate::asset::{AnyBox, Asset, TypeId, UnregisteredAssetType};
use std::collections::HashMap;

/// Runtime listings of the kinds of assets that exist.
/// Each asset type corresponds one-to-one with an implementation of the [`Asset`](crate::asset::Asset) trait.
pub struct TypeRegistry {
	registrations: HashMap<TypeId, Registration>,
}

impl Default for TypeRegistry {
	fn default() -> Self {
		Self {
			registrations: HashMap::new(),
		}
	}
}

impl TypeRegistry {
	pub fn get() -> &'static std::sync::RwLock<TypeRegistry> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<TypeRegistry> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn read() -> std::sync::RwLockReadGuard<'static, TypeRegistry> {
		TypeRegistry::get().read().unwrap()
	}
}

impl TypeRegistry {
	pub fn register<T>(&mut self)
	where
		T: Asset,
	{
		assert!(!self.registrations.contains_key(T::asset_type()));
		self.registrations
			.insert(T::asset_type(), Registration::from::<T>());
	}

	pub fn at(&self, type_id: &str) -> Result<&Registration, UnregisteredAssetType> {
		self.registrations
			.get(type_id)
			.ok_or(UnregisteredAssetType(type_id.to_string()))
	}
}

pub struct Registration {
	fn_decompile: Box<dyn Fn(&Vec<u8>) -> anyhow::Result<AnyBox> + 'static + Send + Sync>,
}
impl Registration {
	fn from<T>() -> Self
	where
		T: Asset,
	{
		Self {
			fn_decompile: Box::new(|bytes| T::decompile(bytes)),
		}
	}

	pub fn decompile(&self, bytes: &Vec<u8>) -> anyhow::Result<AnyBox> {
		(self.fn_decompile)(bytes)
	}
}

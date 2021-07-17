use super::{Kind, KindId, Registration};
use crate::utility;
use std::sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

pub struct Registry {
	types: utility::Registry<KindId, Registration>,
}

impl Default for Registry {
	fn default() -> Self {
		Self {
			types: utility::Registry::new(),
		}
	}
}

impl Registry {
	pub fn get() -> &'static RwLock<Registry> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<Registry> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}

	pub fn register<T>()
	where
		T: Kind + 'static,
	{
		if let Ok(mut registry) = Self::write() {
			registry.types.register::<T>();
		}
	}

	pub fn at(&self, id: &str) -> Option<&Registration> {
		self.types.types.get(id)
	}
}

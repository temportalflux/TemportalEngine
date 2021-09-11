use super::mode;
use std::sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

#[derive(Debug)]
pub struct Config {
	pub mode: mode::Set,
	pub port: u16,
}

#[derive(Default)]
pub struct Network {}

impl Network {
	fn get() -> &'static RwLock<Network> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<Network> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}
}

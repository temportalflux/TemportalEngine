pub use input_actions::*;
use std::sync::{RwLock, RwLockReadGuard, RwLockWriteGuard};

pub fn get() -> &'static RwLock<System> {
	use crate::utility::singleton::*;
	static mut INSTANCE: Singleton<System> = Singleton::uninit();
	unsafe { INSTANCE.get_init(System::new) }
}

pub fn read() -> RwLockReadGuard<'static, System> {
	get().read().unwrap()
}

pub fn write() -> RwLockWriteGuard<'static, System> {
	get().write().unwrap()
}

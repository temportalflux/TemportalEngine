use super::mode;
use std::sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

#[derive(Debug)]
pub struct Config {
	pub mode: mode::Set,
	pub port: u16,
}

#[derive(Default)]
pub struct Network {}

/*
				Event::Connected(address) => {
					if let Ok(mut network) = Self::write() {
						let conn_id = network.connection_list.add_connection(&address);
						log::info!(target: LOG, "{} has connected as {}", address, conn_id);
					}
				}
				Event::TimedOut(_address) => {}
				Event::Disconnected(address) => {
					if let Ok(mut network) = Self::write() {
						if let Some(conn_id) = network.connection_list.remove_connection(&address) {
							log::info!(target: LOG, "{} has disconnected as {}", address, conn_id);
						}
					}
				}

*/

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

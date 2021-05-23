use crate::input::*;
use std::{
	collections::HashMap,
	sync::{RwLock, RwLockReadGuard, RwLockWriteGuard},
};

#[derive(Debug)]
pub struct System {
	key_states: HashMap<KeyCode, (ButtonState, std::time::Instant)>,
}

impl Default for System {
	fn default() -> Self {
		Self {
			key_states: HashMap::new(),
		}
	}
}

impl System {
	pub fn get() -> &'static RwLock<Self> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<System> = Singleton::uninit();
		unsafe { INSTANCE.get() }
	}

	pub fn read() -> RwLockReadGuard<'static, Self> {
		Self::get().read().unwrap()
	}

	pub fn write() -> RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}
}

impl System {
	pub fn send_event(&mut self, sys_event: SystemEvent) {
		match sys_event {
			SystemEvent::Key(keycode, state) => {
				self.key_states
					.insert(keycode, (state, std::time::Instant::now()));
			}
			_ => {}
		}
	}

	pub fn is_key_pressed(&self, keycode: KeyCode, time_since_pressed: std::time::Duration) -> bool {
		match self.key_states.get(&keycode) {
			Some(state_time) => std::time::Instant::now() - state_time.1 <= time_since_pressed,
			None => false,
		}
	}
}

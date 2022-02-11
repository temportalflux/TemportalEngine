pub use input_actions::*;
use std::{
	mem::MaybeUninit,
	sync::{Arc, Once, RwLock},
};

fn config() -> &'static mut Option<Arc<RwLock<Config>>> {
	static mut INSTANCE: Option<Arc<RwLock<Config>>> = None;
	unsafe { &mut INSTANCE }
}

pub fn set_config(configuration: Config) {
	*config() = Some(Arc::new(RwLock::new(configuration)));
}

pub(crate) fn device_cache() -> &'static Arc<RwLock<DeviceCache>> {
	static mut INSTANCE: (MaybeUninit<Arc<RwLock<DeviceCache>>>, Once) =
		(MaybeUninit::uninit(), Once::new());
	unsafe {
		INSTANCE
			.1
			.call_once(|| INSTANCE.0.as_mut_ptr().write(Default::default()));
	}

	unsafe { &*INSTANCE.0.as_ptr() }
}

pub fn create_user<T: Into<String>>(name: T) -> ArcLockUser {
	let config = match config().as_ref() {
		Some(config) => Arc::downgrade(&config),
		None => unimplemented!(),
	};
	let mut device_cache = device_cache().write().unwrap();
	let user = User::new(name.into())
		.with_config(config)
		.with_consts(device_cache.consts())
		.arclocked();
	device_cache.add_user(Arc::downgrade(&user));
	user
}

pub(crate) fn send_event(event: event::Event) {
	if let Ok(mut cache) = device_cache().write() {
		cache.send_event(event);
	}
}

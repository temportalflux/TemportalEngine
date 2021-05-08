use std::{
	mem::MaybeUninit,
	sync::{Once, RwLock},
};

pub struct Singleton<T>(MaybeUninit<RwLock<T>>, Once);

impl<T> Singleton<T> {
	pub const fn uninit() -> Singleton<T> {
		Singleton(MaybeUninit::uninit(), Once::new())
	}

	pub fn get(&mut self) -> &'static RwLock<T>
	where
		T: Default,
	{
		let rwlock = &mut self.0;
		let once = &mut self.1;
		once.call_once(|| unsafe { rwlock.as_mut_ptr().write(RwLock::new(T::default())) });
		unsafe { &*self.0.as_ptr() }
	}
}

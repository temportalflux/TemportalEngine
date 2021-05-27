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
		self.get_init(T::default)
	}

	pub fn get_init<F>(&mut self, init: F) -> &'static RwLock<T>
	where
		F: Fn() -> T,
	{
		let rwlock = &mut self.0;
		let once = &mut self.1;
		once.call_once(|| unsafe { rwlock.as_mut_ptr().write(RwLock::new(init())) });
		unsafe { &*self.0.as_ptr() }
	}
}

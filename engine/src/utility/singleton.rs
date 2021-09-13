use std::{
	mem::MaybeUninit,
	sync::{Once, RwLock},
};

pub struct AutoInit<T>(MaybeUninit<RwLock<T>>, Once);
pub type Singleton<T> = AutoInit<T>;

impl<T> AutoInit<T> {
	pub const fn uninit() -> AutoInit<T> {
		AutoInit(MaybeUninit::uninit(), Once::new())
	}

	pub fn get(&mut self) -> &'static RwLock<T> {
		assert!(self.is_initialized());
		unsafe { &*self.0.as_ptr() }
	}

	pub fn get_or_default(&mut self) -> &'static RwLock<T>
	where
		T: Default,
	{
		self.get_or_init(T::default)
	}

	pub fn get_or_init<F>(&mut self, init: F) -> &'static RwLock<T>
	where
		F: Fn() -> T,
	{
		self.init(init);
		self.get()
	}

	pub fn init<F>(&mut self, init: F)
	where
		F: Fn() -> T,
	{
		if !self.is_initialized() {
			self.init_with(init());
		}
	}

	pub fn init_with(&mut self, data: T) {
		let rwlock = &mut self.0;
		let once = &mut self.1;
		once.call_once(|| unsafe { rwlock.as_mut_ptr().write(RwLock::new(data)) });
	}

	pub fn is_initialized(&self) -> bool {
		self.1.is_completed()
	}
}

pub struct RwOptional<T>(MaybeUninit<RwLock<Option<T>>>, Once);
impl<T> RwOptional<T> {
	pub const fn uninit() -> RwOptional<T> {
		RwOptional(MaybeUninit::uninit(), Once::new())
	}

	pub fn get(&mut self) -> &'static RwLock<Option<T>> {
		let rwlock = &mut self.0;
		let once = &mut self.1;
		once.call_once(|| unsafe { rwlock.as_mut_ptr().write(RwLock::new(None)) });
		unsafe { &*self.0.as_ptr() }
	}
}

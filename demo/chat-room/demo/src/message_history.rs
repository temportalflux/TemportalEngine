use std::sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

pub struct Message {
	pub content: String,
}

#[derive(Default)]
pub struct MessageHistory {
	messages: Vec<Message>,
}

impl MessageHistory {
	fn get() -> &'static RwLock<Self> {
		use temportal_engine::utility::singleton::*;
		static mut INSTANCE: Singleton<MessageHistory> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}
}

impl MessageHistory {
	pub fn iter_messages(&self) -> std::slice::Iter<Message> {
		self.messages.iter()
	}

	pub fn add(&mut self, message: Message) {
		self.messages.push(message);
	}
}

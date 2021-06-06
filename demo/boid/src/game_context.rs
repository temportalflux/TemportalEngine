use crate::ecs::systems::{CreateEntityMessage, DestroyEntityMessage};
use std::sync::mpsc::SyncSender;

type CreateBoidSender = SyncSender<CreateEntityMessage>;
type DestroyBoidSender = SyncSender<DestroyEntityMessage>;

#[derive(Default)]
pub struct GameContext {
	create_boid: Option<CreateBoidSender>,
	destroy_boid: Option<DestroyBoidSender>,
}

impl GameContext {
	pub fn get() -> &'static std::sync::RwLock<Self> {
		use crate::engine::utility::singleton::*;
		static mut INSTANCE: Singleton<GameContext> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> std::sync::RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}

	pub fn _read() -> std::sync::RwLockReadGuard<'static, Self> {
		Self::get().read().unwrap()
	}
}

impl GameContext {
	pub fn with_senders(&mut self, create: CreateBoidSender, destroy: DestroyBoidSender) {
		self.create_boid = Some(create);
		self.destroy_boid = Some(destroy);
	}

	pub fn enqueue_create(&mut self, msg: CreateEntityMessage) {
		let _ = self.create_boid.as_mut().unwrap().try_send(msg);
	}

	pub fn enqueue_destroy(&mut self, msg: DestroyEntityMessage) {
		let _ = self.destroy_boid.as_mut().unwrap().try_send(msg);
	}
}

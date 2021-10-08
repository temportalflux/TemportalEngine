use std::sync::{Arc, LockResult, RwLock, RwLockReadGuard};

pub type ArcLockWidget = Arc<RwLock<dyn Widget + 'static + Send + Sync>>;
pub type LockedReadWidget<'a> = LockResult<RwLockReadGuard<'a, dyn Widget + 'static + Send + Sync>>;
pub trait Widget: super::AsRAUI {
	fn arclocked(self) -> Arc<RwLock<Self>>
	where
		Self: Sized,
	{
		Arc::new(RwLock::new(self))
	}
}

pub mod container;

mod text;
pub use text::*;

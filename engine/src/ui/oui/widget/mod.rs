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

mod image_box;
pub use image_box::*;

mod size_box;
pub use size_box::*;

mod text;
pub use text::*;

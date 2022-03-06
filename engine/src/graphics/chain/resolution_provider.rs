use crate::graphics::{device::physical, structs::Extent2D};
use std::sync::{Arc, LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

pub type ArcResolutionProvider = Arc<dyn ResolutionProvider + 'static + Send + Sync>;

pub trait ResolutionProvider {
	fn has_changed(&self) -> bool;
	fn query(&self, physical: &Arc<physical::Device>) -> Extent2D;
}

pub struct DisplayResolution;
impl ResolutionProvider for DisplayResolution {
	fn has_changed(&self) -> bool {
		false
	}

	fn query(&self, physical: &Arc<physical::Device>) -> Extent2D {
		physical.query_surface_support().image_extent()
	}
}

pub struct ProvidedResolution {
	prev: RwLock<Extent2D>,
	next: RwLock<Extent2D>,
}

impl ProvidedResolution {
	pub fn read(&self) -> LockResult<RwLockReadGuard<'_, Extent2D>> {
		self.next.read()
	}

	pub fn write(&self) -> LockResult<RwLockWriteGuard<'_, Extent2D>> {
		self.next.write()
	}
}

impl ResolutionProvider for ProvidedResolution {
	fn has_changed(&self) -> bool {
		*self.next.read().unwrap() != *self.prev.read().unwrap()
	}

	fn query(&self, _physical: &Arc<physical::Device>) -> Extent2D {
		if self.has_changed() {
			*self.prev.write().unwrap() = *self.next.read().unwrap();
		}
		*self.next.read().unwrap()
	}
}

use std::sync::{Arc, Mutex, MutexGuard, RwLock, Weak};

type Map = anymap::Map<dyn anymap::any::Any + 'static + Send + Sync>;
pub struct ValueSet(Mutex<Map>);
impl ValueSet {
	pub fn new() -> Arc<Self> {
		Arc::new(Self(Mutex::new(Map::new())))
	}

	fn locked(&self) -> MutexGuard<'_, Map> {
		self.0.lock().unwrap()
	}

	pub fn is_empty(&self) -> bool {
		self.locked().is_empty()
	}

	pub fn contains<T>(&self) -> bool
	where
		T: 'static + Send + Sync,
	{
		self.locked().contains::<T>()
	}

	pub fn insert<T>(&self, value: T)
	where
		T: 'static + Send + Sync,
	{
		self.locked().insert(value);
	}

	pub fn insert_handle<T>(self: &Arc<Self>, arc: Arc<T>) -> ArcHandle<T> where T: 'static + Send + Sync {
		self.insert(Arc::downgrade(&arc));
		ArcHandle(Arc::downgrade(&self), arc)
	}

	pub fn remove<T>(&self) -> Option<T>
	where
		T: 'static + Send + Sync,
	{
		self.locked().remove::<T>()
	}

	pub fn get<T>(&self) -> Option<T>
	where
		T: 'static + Send + Sync + Clone,
	{
		self.locked().get::<T>().cloned()
	}

	pub fn get_arc<T>(&self) -> Option<Arc<T>>
	where
		T: 'static + Send + Sync,
	{
		self.get::<Arc<T>>()
	}

	pub fn get_weak<T>(&self) -> Option<Weak<T>>
	where
		T: 'static + Send + Sync,
	{
		self.get::<Weak<T>>()
	}

	pub fn get_weak_upgraded<T>(&self) -> Option<Arc<T>>
	where
		T: 'static + Send + Sync,
	{
		self.get_weak::<T>().map(|weak| weak.upgrade()).flatten()
	}

	pub fn get_arclock<T>(&self) -> Option<Arc<RwLock<T>>>
	where
		T: 'static + Send + Sync,
	{
		self.get_arc::<RwLock<T>>()
	}

	pub fn get_weaklock<T>(&self) -> Option<Weak<RwLock<T>>>
	where
		T: 'static + Send + Sync,
	{
		self.get_weak::<RwLock<T>>()
	}

	pub fn get_weaklock_upgraded<T>(&self) -> Option<Arc<RwLock<T>>>
	where
		T: 'static + Send + Sync,
	{
		self.get_weak_upgraded::<RwLock<T>>()
	}
}

pub struct ArcHandle<T: 'static + Send + Sync>(Weak<ValueSet>, Arc<T>);
impl<T> ArcHandle<T> where T: 'static + Send + Sync {
	pub fn inner(&self) -> &Arc<T> {
		&self.1
	}

	pub fn into_inner(self) -> Arc<T> {
		self.1.clone()
	}
}
impl<T> Drop for ArcHandle<T> where T: 'static + Send + Sync {
	fn drop(&mut self) {
		if let Some(set) = self.0.upgrade() {
			set.remove::<Weak<T>>();
		}
	}
}

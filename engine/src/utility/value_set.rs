use std::{
	any::TypeId,
	collections::HashMap,
	sync::{Arc, Mutex, MutexGuard, RwLock, Weak},
};

type Map = anymap::Map<dyn anymap::any::Any + 'static + Send + Sync>;
type Inner = (Map, HashMap<TypeId, &'static str>);
pub struct ValueSet(Mutex<Inner>);
impl ValueSet {
	pub fn new() -> Arc<Self> {
		Arc::new(Self(Mutex::new((Map::new(), HashMap::new()))))
	}

	fn locked(&self) -> MutexGuard<'_, Inner> {
		self.0.lock().unwrap()
	}

	pub fn is_empty(&self) -> bool {
		self.locked().0.is_empty()
	}

	pub fn contains<T>(&self) -> bool
	where
		T: 'static + Send + Sync,
	{
		self.locked().0.contains::<T>()
	}

	pub fn insert<T>(&self, value: T)
	where
		T: 'static + Send + Sync,
	{
		let mut locked = self.locked();
		locked.0.insert(value);
		locked
			.1
			.insert(TypeId::of::<T>(), std::any::type_name::<T>());
	}

	pub fn insert_handle<T>(self: &Arc<Self>, arc: Arc<T>) -> ArcHandle<T>
	where
		T: 'static + Send + Sync,
	{
		self.insert(Arc::downgrade(&arc));
		ArcHandle(Arc::downgrade(&self), arc)
	}

	pub fn remove<T>(&self) -> Option<T>
	where
		T: 'static + Send + Sync,
	{
		let mut locked = self.locked();
		let value = locked.0.remove::<T>();
		locked.1.remove(&TypeId::of::<T>());
		value
	}

	pub fn get<T>(&self) -> Option<T>
	where
		T: 'static + Send + Sync + Clone,
	{
		self.locked().0.get::<T>().cloned()
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

impl std::fmt::Debug for ValueSet {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		let mut names = self.locked().1.values().map(|v| *v).collect::<Vec<_>>();
		names.sort();
		write!(f, "ValueSet({:?})", names)
	}
}

pub struct ArcHandle<T: 'static + Send + Sync>(Weak<ValueSet>, Arc<T>);
impl<T> ArcHandle<T>
where
	T: 'static + Send + Sync,
{
	pub fn inner(&self) -> &Arc<T> {
		&self.1
	}

	pub fn into_inner(self) -> Arc<T> {
		self.1.clone()
	}
}
impl<T> Drop for ArcHandle<T>
where
	T: 'static + Send + Sync,
{
	fn drop(&mut self) {
		if let Some(set) = self.0.upgrade() {
			set.remove::<Weak<T>>();
		}
	}
}

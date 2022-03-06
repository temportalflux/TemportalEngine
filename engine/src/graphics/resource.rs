use crate::graphics::{
	command::{self, frame::AttachedView},
	procedure::Attachment,
	Chain,
};
use std::{
	any::Any,
	collections::HashMap,
	sync::{Arc, RwLock, Weak},
};

mod depth_buffer;
pub use depth_buffer::*;
mod color_buffer;
pub use color_buffer::*;

pub type Id = &'static str;

pub trait Resource {
	fn unique_id() -> Id
	where
		Self: Sized;

	fn construct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	fn get_attachments(&self) -> Vec<(Weak<Attachment>, AttachedView)> {
		Vec::new()
	}
}

type AnyResource = Arc<dyn Any + 'static + Send + Sync>;
type ArcResource = Arc<RwLock<dyn Resource + 'static + Send + Sync>>;

struct Registration {
	arc_trait: ArcResource,
	arc_any: AnyResource,
}
impl Registration {
	fn new<T>(resource: T) -> Self
	where
		T: Resource + 'static + Send + Sync,
	{
		let arc = Arc::new(RwLock::new(resource));
		Self {
			arc_trait: arc.clone(),
			arc_any: arc.clone(),
		}
	}

	fn get_trait(&self) -> &ArcResource {
		&self.arc_trait
	}

	fn get_typed<T>(&self) -> Option<Arc<RwLock<T>>>
	where
		T: Resource + 'static + Send + Sync,
	{
		self.arc_any.clone().downcast::<RwLock<T>>().ok()
	}
}

#[derive(Default)]
pub struct Registry {
	resources: HashMap<Id, Registration>,
	order: Vec<Id>,
}

impl Registry {
	pub fn add<T>(&mut self, resource: T) -> &mut Self
	where
		T: Resource + 'static + Send + Sync,
	{
		assert!(!self.resources.contains_key(T::unique_id()));
		self.resources
			.insert(T::unique_id(), Registration::new(resource));
		self.order.push(T::unique_id());
		self
	}

	pub fn get<T>(&self) -> Result<Arc<RwLock<T>>, NoSuchResource>
	where
		T: Resource + 'static + Send + Sync,
	{
		self.resources
			.get(T::unique_id())
			.map(|reg| reg.get_typed())
			.flatten()
			.ok_or(NoSuchResource(T::unique_id()))
	}

	pub fn iter(&self) -> impl std::iter::Iterator<Item = &ArcResource> {
		self.order
			.iter()
			.filter_map(|id| self.resources.get(id))
			.map(|reg| reg.get_trait())
			.collect::<Vec<_>>()
			.into_iter()
	}
}

#[derive(thiserror::Error, Debug)]
pub struct NoSuchResource(&'static str);
impl std::fmt::Display for NoSuchResource {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"No resource on the graphics chain with the type id \"{}\".",
			self.0
		)
	}
}

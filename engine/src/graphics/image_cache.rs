use crate::{
	graphics::{
		self, command, flags, image_view, sampler, structs,
		utility::{NameableBuilder, NamedObject},
		Texture,
	},
	math::nalgebra::Vector2,
};
use std::{collections::HashMap, sync};

pub struct PendingEntry {
	name: Option<String>,
	compiled: graphics::CompiledTexture,
	format: flags::format::Format,
	sampler: graphics::sampler::Builder,
}

impl PendingEntry {
	pub fn new() -> Self {
		Self {
			name: None,
			compiled: graphics::CompiledTexture {
				size: [0, 0].into(),
				binary: Vec::new(),
			},
			format: flags::format::Format::UNDEFINED,
			sampler: graphics::sampler::Builder::default(),
		}
	}

	pub fn with_name<T>(mut self, name: T) -> Self
	where
		T: Into<String>,
	{
		self.name = Some(name.into());
		self
	}

	pub fn with_size(mut self, size: Vector2<usize>) -> Self {
		self.compiled.size = size;
		self
	}

	pub fn with_binary(mut self, binary: Vec<u8>) -> Self {
		self.compiled.binary = binary;
		self
	}

	pub fn with_format(mut self, format: flags::format::Format) -> Self {
		self.format = format;
		self
	}

	pub fn sampler_mut(&mut self) -> &mut graphics::sampler::Builder {
		&mut self.sampler
	}

	pub fn init_sampler<F>(mut self, init: F) -> Self
	where
		F: Fn(&mut graphics::sampler::Builder),
	{
		init(&mut self.sampler);
		self
	}
}

/// The GPU objects to view and sample from an image.
/// Created by [`ImageCache::load_pending`].
pub struct CombinedImageSampler {
	pub view: sync::Arc<image_view::View>,
	pub sampler: sync::Arc<sampler::Sampler>,
}

/// A collection of GPU [`Image`](graphics::image::Image), [`View`](graphics::image_view::View),
/// and [`Sampler`](graphics::sampler::Sampler) objects, keyed by a user-facing identifier.
/// Useful when a collection of images are used frequently by multiple draw calls but in the same pipeline,
/// or when managing any collection of engine asset textures.
pub struct ImageCache<T: Eq + std::hash::Hash + Clone> {
	pending: HashMap<T, PendingEntry>,
	loaded: HashMap<T, CombinedImageSampler>,
	name: Option<String>,
}

impl<T> Default for ImageCache<T>
where
	T: Eq + std::hash::Hash + Clone,
{
	fn default() -> Self {
		Self {
			loaded: HashMap::new(),
			pending: HashMap::new(),
			name: None,
		}
	}
}

impl<T> ImageCache<T>
where
	T: Eq + std::hash::Hash + Clone,
{
	pub fn with_cache_name<TStr>(mut self, name: TStr) -> Self
	where
		TStr: Into<String>,
	{
		self.set_cache_name(Some(name.into()));
		self
	}

	pub fn set_cache_name(&mut self, name: Option<String>) {
		self.name = name;
	}

	/// Adds an engine asset texture to the cache,
	/// to be created the next time [`load_pending`](ImageCache::load_pending) is executed.
	pub fn insert(&mut self, id: T, name: Option<String>, texture: Box<Texture>) {
		self.insert_compiled(
			id,
			name,
			*texture.size(),
			flags::format::SRGB_8BIT,
			texture.binary().clone(),
		);
	}

	pub fn insert_compiled(
		&mut self,
		id: T,
		name: Option<String>,
		size: Vector2<usize>,
		format: flags::format::Format,
		binary: Vec<u8>,
	) {
		self.pending.insert(
			id,
			PendingEntry {
				name,
				compiled: graphics::CompiledTexture { size, binary },
				format,
				sampler: graphics::sampler::Builder::default()
					.with_magnification(flags::Filter::NEAREST)
					.with_minification(flags::Filter::NEAREST)
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3]),
			},
		);
	}

	pub fn insert_pending(&mut self, id: T, entry: PendingEntry) {
		self.pending.insert(id, entry);
	}

	/// Returns true if the `id` has been added via [`insert`](ImageCache::insert),
	/// regardless of if [`load_pending`](ImageCache::load_pending) has been executed since insertion or not.
	pub fn contains(&self, id: &T) -> bool {
		self.loaded.contains_key(id) || self.pending.contains_key(id)
	}

	/// Loads all pending images added by [`insert`](ImageCache::insert),
	/// creating an [`Image`](graphics::image::Image), [`View`](graphics::image_view::View),
	/// and [`Sampler`](graphics::sampler::Sampler),
	/// and sending the texture binary data to the GPU - for each image inserted.
	///
	/// Returns a tuple of:
	///
	/// a. the ids which were loaded. The user can get the [`CombinedImageSampler`]
	///    for the id by using the index operator on this [`ImageCache[id]`](ImageCache).
	///
	/// b. list of [`semaphores`](command::Semaphore) which will
	///    be signaled when the GPU has finished writing the image data.
	///    These returned semaphores must be waited on by any commands which read from the image.
	#[profiling::function]
	pub fn load_pending(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> anyhow::Result<(Vec<(T, Option<String>)>, Vec<sync::Arc<command::Semaphore>>)> {
		let mut ids = Vec::new();
		let mut pending_gpu_signals = Vec::new();
		if !self.pending.is_empty() {
			// Collect all the pending items into a vector, draining the member so that they arent created ever again.
			let pending_images = self.pending.drain().collect::<Vec<_>>();
			// Load/Create the image on GPU for each pending item
			for (id, pending) in pending_images.into_iter() {
				let (loaded, mut signals) = self.create_image(render_chain, &pending)?;
				// promote the required signals so they can be returned by `load_pending`
				pending_gpu_signals.append(&mut signals);
				ids.push((id.clone(), pending.name.clone()));
				// insert the image into the collection, thereby dropping any item with the same id
				self.loaded.insert(id, loaded);
			}
		}

		Ok((ids, pending_gpu_signals))
	}

	fn make_object_name(&self, name: &Option<String>, suffix: &str) -> Option<String> {
		match (self.name.as_ref(), name) {
			(Some(cache_name), Some(name)) => Some(format!("{}.{}.{}", cache_name, name, suffix)),
			_ => None,
		}
	}

	/// Creates the image, view, and sampler for each pending item.
	/// Returns the loaded struct, and a list of semaphores which will
	/// be signaled when the GPU has finished writing the image data.
	/// These returned semaphores must be signaled before the GPU can read from the image.
	#[profiling::function]
	fn create_image(
		&self,
		render_chain: &graphics::RenderChain,
		pending: &PendingEntry,
	) -> anyhow::Result<(CombinedImageSampler, Vec<sync::Arc<command::Semaphore>>)> {
		use graphics::{
			image, structs::subresource, utility::BuildFromDevice, GpuOperationBuilder,
		};

		let mut signals = Vec::new();

		let image = sync::Arc::new(image::Image::create_gpu(
			&render_chain.allocator(),
			self.make_object_name(&pending.name, "Image"),
			pending.format,
			structs::Extent3D {
				width: pending.compiled.size.x as u32,
				height: pending.compiled.size.y as u32,
				depth: 1,
			},
		)?);

		GpuOperationBuilder::new(image.wrap_name(|v| format!("Create({})", v)), render_chain)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&pending.compiled.binary[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.add_signal_to(&mut signals)
			.end()?;

		let view = sync::Arc::new(
			image_view::View::builder()
				.with_optname(self.make_object_name(&pending.name, "Image.View"))
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&render_chain.logical())?,
		);

		let sampler = sync::Arc::new(
			pending
				.sampler
				.clone()
				.with_optname(self.make_object_name(&pending.name, "Image.Sampler"))
				.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
				.build(&render_chain.logical())?,
		);

		Ok((CombinedImageSampler { view, sampler }, signals))
	}
}

impl<T> std::ops::Index<&T> for ImageCache<T>
where
	T: Eq + std::hash::Hash + Clone,
{
	type Output = CombinedImageSampler;
	fn index(&self, id: &T) -> &Self::Output {
		&self.loaded[id]
	}
}

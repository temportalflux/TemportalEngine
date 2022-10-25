use crate::channels::mpsc::Sender;
use crate::{
	graphics::{
		self, command, flags, image_view, sampler, structs,
		utility::{NameableBuilder, NamedObject},
		GpuOpContext, Texture,
	},
	math::nalgebra::Vector2,
};
use multimap::MultiMap;
use std::{
	collections::HashMap,
	sync::{self, Arc},
};
use vulkan_rs::image::Image;
use vulkan_rs::structs::{Extent3D, Offset3D};

pub enum PendingOperation {
	Create(PendingImage),
	Write(ImageBinary),
}

pub struct PendingImage {
	name: String,
	size: Vector2<usize>,
	format: flags::format::Format,
	sampler: graphics::sampler::Builder,
}

pub struct ImageBinary {
	offset: Option<Vector2<usize>>,
	binary: Vec<u8>,
	size: Vector2<usize>,
}

impl Default for PendingImage {
	fn default() -> Self {
		Self {
			name: String::new(),
			size: [0, 0].into(),
			format: flags::format::Format::UNDEFINED,
			sampler: graphics::sampler::Builder::default(),
		}
	}
}

impl PendingImage {
	pub fn with_name<T>(mut self, name: T) -> Self
	where
		T: Into<String>,
	{
		self.name = name.into();
		self
	}

	pub fn with_size(mut self, size: Vector2<usize>) -> Self {
		self.size = size;
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

impl ImageBinary {
	pub fn from(size: Vector2<usize>, binary: Vec<u8>) -> Self {
		Self {
			offset: None,
			binary,
			size,
		}
	}

	pub fn with_offset(mut self, offset: Option<Vector2<usize>>) -> Self {
		self.offset = offset;
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
	pending: MultiMap<T, PendingOperation>,
	loaded: HashMap<T, CombinedImageSampler>,
	name: String,
}

impl<T> Default for ImageCache<T>
where
	T: Eq + std::hash::Hash + Clone,
{
	fn default() -> Self {
		Self {
			loaded: HashMap::new(),
			pending: MultiMap::new(),
			name: String::new(),
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
		self.set_cache_name(name.into());
		self
	}

	pub fn set_cache_name(&mut self, name: String) {
		self.name = name;
	}

	/// Adds an engine asset texture to the cache,
	/// to be created the next time [`load_pending`](ImageCache::load_pending) is executed.
	pub fn insert_asset(&mut self, id: T, name: String, texture: Box<Texture>) {
		self.insert_compiled(
			id,
			name,
			*texture.size(),
			flags::format::SRGB_8BIT,
			texture.binary().clone(),
		);
	}

	fn insert_compiled(
		&mut self,
		id: T,
		name: String,
		size: Vector2<usize>,
		format: flags::format::Format,
		binary: Vec<u8>,
	) {
		self.pending.insert(
			id.clone(),
			PendingOperation::Create(PendingImage {
				name,
				format,
				size: size,
				sampler: graphics::sampler::Builder::default()
					.with_magnification(flags::Filter::NEAREST)
					.with_minification(flags::Filter::NEAREST)
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3]),
			}),
		);
		self.pending.insert(
			id,
			PendingOperation::Write(ImageBinary {
				offset: None,
				binary,
				size,
			}),
		);
	}

	pub fn insert_operation(&mut self, id: T, entry: PendingOperation) {
		self.pending.insert(id, entry);
	}

	/// Returns true if the `id` has been added via [`insert`](ImageCache::insert),
	/// regardless of if [`load_pending`](ImageCache::load_pending) has been executed since insertion or not.
	pub fn contains(&self, id: &T) -> bool {
		self.loaded.contains_key(id) || self.pending.contains_key(id)
	}

	pub fn remove(&mut self, id: &T) -> Option<CombinedImageSampler> {
		self.loaded.remove(id)
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
	pub fn process_operations(
		&mut self,
		context: &impl GpuOpContext,
		signal_sender: &Sender<Arc<command::Semaphore>>,
	) -> anyhow::Result<Vec<(T, String)>> {
		let mut ids = Vec::new();
		if !self.pending.is_empty() {
			let keys = self.pending.keys().cloned().collect::<Vec<_>>();
			for id in keys.into_iter() {
				let ops = self.pending.remove(&id).unwrap();
				for op in ops.into_iter() {
					if let Some(out) = self.process_op(context, signal_sender, &id, op)? {
						ids.push(out);
					}
				}
			}
		}
		Ok(ids)
	}

	fn process_op(
		&mut self,
		context: &impl GpuOpContext,
		signal_sender: &Sender<Arc<command::Semaphore>>,
		id: &T,
		op: PendingOperation,
	) -> anyhow::Result<Option<(T, String)>> {
		match op {
			PendingOperation::Create(pending) => {
				let loaded = self.create_image(context, &pending)?;
				// insert the image into the collection, thereby dropping any item with the same id
				self.loaded.insert(id.clone(), loaded);
				Ok(Some((id.clone(), pending.name.clone())))
			}
			PendingOperation::Write(data) => {
				let loaded = self.loaded.get(id).unwrap();
				let image = loaded.view.image();
				self.write_image(context, signal_sender, image, data)?;
				Ok(None)
			}
		}
	}

	fn make_object_name(&self, name: &String, suffix: &str) -> String {
		format!("{}.{}.{}", self.name, name, suffix)
	}

	fn write_image(
		&self,
		context: &impl GpuOpContext,
		signal_sender: &Sender<Arc<command::Semaphore>>,
		image: &Arc<Image>,
		data: ImageBinary,
	) -> anyhow::Result<()> {
		use graphics::{command::CopyBufferToImage, structs::subresource, GpuOperationBuilder};
		GpuOperationBuilder::new(format!("Create({})", image.name()), context)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&data.binary[..])?
			.copy_stage_to_image_regions(
				&image,
				vec![CopyBufferToImage {
					buffer_offset: 0,
					layers: subresource::Layers::default().with_aspect(flags::ImageAspect::COLOR),
					offset: data
						.offset
						.map(|vec| Offset3D {
							x: vec.x as i32,
							y: vec.y as i32,
							z: 0i32,
						})
						.unwrap_or_default(),
					size: Extent3D {
						width: data.size.x as u32,
						height: data.size.y as u32,
						depth: 1u32,
					},
				}],
			)
			.format_image_for_read(&image)
			.send_signal_to(signal_sender)?
			.end()?;
		Ok(())
	}

	/// Creates the image, view, and sampler for each pending item.
	/// Returns the loaded struct, and a list of semaphores which will
	/// be signaled when the GPU has finished writing the image data.
	/// These returned semaphores must be signaled before the GPU can read from the image.
	#[profiling::function]
	fn create_image(
		&self,
		context: &impl GpuOpContext,
		pending: &PendingImage,
	) -> anyhow::Result<CombinedImageSampler> {
		use graphics::{image, structs::subresource, utility::BuildFromDevice};

		let image = sync::Arc::new(image::Image::create_gpu(
			&context.object_allocator()?,
			self.make_object_name(&pending.name, "Image"),
			pending.format,
			structs::Extent3D {
				width: pending.size.x as u32,
				height: pending.size.y as u32,
				depth: 1,
			},
		)?);

		let view = sync::Arc::new(
			image_view::View::builder()
				.with_name(self.make_object_name(&pending.name, "Image.View"))
				.for_image(image)
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&context.logical_device()?)?,
		);

		let sampler = sync::Arc::new(
			pending
				.sampler
				.clone()
				.with_name(self.make_object_name(&pending.name, "Image.Sampler"))
				.with_max_anisotropy(Some(context.physical_device()?.max_sampler_anisotropy()))
				.build(&context.logical_device()?)?,
		);

		Ok(CombinedImageSampler { view, sampler })
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

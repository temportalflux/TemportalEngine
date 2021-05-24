use crate::{
	graphics::{self, command, flags, image_view, sampler, structs, Texture},
	task,
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

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
pub struct ImageCache {
	pending: HashMap<String, graphics::CompiledTexture>,
	loaded: HashMap<String, CombinedImageSampler>,
}

impl Default for ImageCache {
	fn default() -> Self {
		Self {
			loaded: HashMap::new(),
			pending: HashMap::new(),
		}
	}
}

impl ImageCache {
	/// Adds an engine asset texture to the cache,
	/// to be created the next time [`load_pending`](ImageCache::load_pending) is executed.
	pub fn insert(&mut self, id: String, texture: Box<Texture>) -> VoidResult {
		self.pending.insert(id, texture.get_compiled().clone());
		Ok(())
	}

	/// Returns true if the `id` has been added via [`insert`](ImageCache::insert),
	/// regardless of if [`load_pending`](ImageCache::load_pending) has been executed since insertion or not.
	pub fn contains(&self, id: &String) -> bool {
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
	) -> utility::Result<(Vec<String>, Vec<sync::Arc<command::Semaphore>>)> {
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
				ids.push(id.clone());
				// insert the image into the collection, thereby dropping any item with the same id
				self.loaded.insert(id, loaded);
			}
		}

		Ok((ids, pending_gpu_signals))
	}

	/// Creates the image, view, and sampler for each pending item.
	/// Returns the loaded struct, and a list of semaphores which will
	/// be signaled when the GPU has finished writing the image data.
	/// These returned semaphores must be signaled before the GPU can read from the image.
	#[profiling::function]
	fn create_image(
		&self,
		render_chain: &graphics::RenderChain,
		pending: &graphics::CompiledTexture,
	) -> utility::Result<(CombinedImageSampler, Vec<sync::Arc<command::Semaphore>>)> {
		use graphics::{image, structs::subresource, TaskGpuCopy};

		let mut signals = Vec::new();

		let image = sync::Arc::new(image::Image::create_gpu(
			&render_chain.allocator(),
			flags::Format::R8G8B8A8_SRGB,
			structs::Extent3D {
				width: pending.size.x() as u32,
				height: pending.size.y() as u32,
				depth: 1,
			},
		)?);

		TaskGpuCopy::new(&render_chain)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&pending.binary[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.end()?
			.add_signal_to(&mut signals)
			.send_to(task::sender());

		let view = sync::Arc::new(
			image_view::View::builder()
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&render_chain.logical())?,
		);

		let sampler = sync::Arc::new(
			graphics::sampler::Sampler::builder()
				.with_magnification(flags::Filter::NEAREST)
				.with_minification(flags::Filter::NEAREST)
				.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
				.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
				.build(&render_chain.logical())?,
		);

		Ok((CombinedImageSampler { view, sampler }, signals))
	}
}

impl std::ops::Index<&String> for ImageCache {
	type Output = CombinedImageSampler;
	fn index(&self, id: &String) -> &Self::Output {
		&self.loaded[id]
	}
}

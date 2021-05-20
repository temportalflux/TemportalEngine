use crate::{
	asset,
	graphics::{self, command, flags, image_view, sampler, Texture},
	task,
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

pub struct CombinedImageSampler {
	pub view: sync::Arc<image_view::View>,
	pub sampler: sync::Arc<sampler::Sampler>,
}

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
	pub fn insert(&mut self, id: &asset::Id, texture: Box<Texture>) -> VoidResult {
		self.pending
			.insert(id.to_str().to_owned(), texture.get_compiled().clone());
		Ok(())
	}

	pub fn contains(&self, id: &String) -> bool {
		self.loaded.contains_key(id) || self.pending.contains_key(id)
	}

	#[profiling::function]
	pub fn load_pending(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<(Vec<String>, Vec<sync::Arc<command::Semaphore>>)> {
		let mut ids = Vec::new();
		let mut pending_gpu_signals = Vec::new();
		if !self.pending.is_empty() {
			let pending_images = self.pending.drain().collect::<Vec<_>>();
			for (id, pending) in pending_images.into_iter() {
				let (loaded, mut signals) = self.create_image(render_chain, &pending)?;
				pending_gpu_signals.append(&mut signals);
				ids.push(id.clone());
				self.loaded.insert(id, loaded);
			}
		}
		Ok((ids, pending_gpu_signals))
	}

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
			pending.size.subvec::<3>(None).with_z(1),
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
				.with_format(image.format())
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

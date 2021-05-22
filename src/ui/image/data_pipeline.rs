use crate::{
	asset,
	graphics::{
		self, command, descriptor, flags, pipeline, structs, DescriptorCache, Drawable, ImageCache,
		Texture,
	},
	ui::{image, mesh},
	utility::{self, VoidResult},
};
use std::sync;

pub struct DataPipeline {
	descriptor_cache: DescriptorCache<String>,
	image_cache: ImageCache,
	drawable: Drawable,
}

impl DataPipeline {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		Ok(Self {
			drawable: Drawable::default(),
			descriptor_cache: DescriptorCache::new(
				descriptor::SetLayout::builder()
					.with_binding(
						0,
						flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
						1,
						flags::ShaderKind::Fragment,
					)
					.build(&render_chain.logical())?,
			),
			image_cache: ImageCache::default(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		self.drawable.add_shader(id)
	}

	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		self.drawable.create_shaders(render_chain)
	}

	pub fn add_pending(&mut self, id: &asset::Id, texture: Box<Texture>) -> VoidResult {
		self.image_cache.insert(id.to_str().to_owned(), texture)
	}

	#[profiling::function]
	pub fn create_pending_images(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		use graphics::descriptor::*;

		let mut pending_gpu_signals = Vec::new();
		let (image_ids, mut signals) = self.image_cache.load_pending(render_chain)?;
		pending_gpu_signals.append(&mut signals);

		for image_id in image_ids.into_iter() {
			let cached_image = &self.image_cache[&image_id];
			let descriptor_set = self.descriptor_cache.insert(image_id, render_chain)?;
			SetUpdate::default()
				.with(UpdateOperation::Write(WriteOp {
					destination: UpdateOperationSet {
						set: descriptor_set.clone(),
						binding_index: 0,
						array_element: 0,
					},
					kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					object: ObjectKind::Image(vec![ImageKind {
						sampler: cached_image.sampler.clone(),
						view: cached_image.view.clone(),
						layout: flags::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					}]),
				}))
				.apply(&render_chain.logical());
		}

		Ok(pending_gpu_signals)
	}

	#[profiling::function]
	pub fn destroy_pipeline(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		self.drawable.destroy_pipeline(render_chain)
	}

	#[profiling::function]
	pub fn create_pipeline(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()> {
		self.drawable.create_pipeline(
			render_chain,
			Some(self.descriptor_cache.layout()),
			pipeline::Info::default()
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(pipeline::ViewportState::from(resolution))
				.set_color_blending(
					pipeline::ColorBlendState::default()
						.add_attachment(pipeline::ColorBlendAttachment::default()),
				)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
		)
	}

	pub fn has_image(&self, image_id: &image::Id) -> bool {
		self.image_cache.contains(image_id) && self.descriptor_cache.contains(image_id)
	}

	pub fn bind_pipeline(&self, buffer: &mut command::Buffer) {
		self.drawable.bind_pipeline(buffer)
	}

	pub fn bind_texture(&self, buffer: &mut command::Buffer, image_id: &image::Id) {
		assert!(self.descriptor_cache.contains(image_id));
		self.drawable.bind_descriptors(
			buffer,
			vec![&self.descriptor_cache[image_id].upgrade().unwrap()],
		);
	}
}

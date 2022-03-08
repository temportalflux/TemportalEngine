use crate::{
	asset,
	graphics::{
		self, command, descriptor,
		device::logical,
		flags, pipeline,
		utility::{BuildFromDevice, NameableBuilder},
		Chain, DescriptorCache, Drawable, GpuOpContext, ImageCache, Texture,
	},
	ui::core::{image, mesh},
};
use anyhow::Result;
use std::sync::Arc;

pub struct DataPipeline {
	descriptor_cache: DescriptorCache<String>,
	image_cache: ImageCache<String>,
	drawable: Drawable,
}

impl DataPipeline {
	pub fn new(context: &impl GpuOpContext) -> anyhow::Result<Self> {
		Ok(Self {
			drawable: Drawable::default().with_name("UI.Image"),
			descriptor_cache: DescriptorCache::new(
				descriptor::layout::SetLayout::builder()
					.with_name("UI.Image.DescriptorLayout")
					.with_binding(
						0,
						flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
						1,
						flags::ShaderKind::Fragment,
					)
					.build(&context.logical_device()?)?,
			),
			image_cache: ImageCache::default().with_cache_name("UI.Image"),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> Result<()> {
		self.drawable.add_shader(id)
	}

	pub fn create_shaders(&mut self, logical: &Arc<logical::Device>) -> anyhow::Result<()> {
		self.drawable.create_shaders(logical)
	}

	pub fn add_pending(&mut self, id: &asset::Id, texture: Box<Texture>) {
		self.image_cache.insert(id.name(), Some(id.name()), texture);
	}

	pub fn contains(&self, id: &asset::Id) -> bool {
		self.image_cache.contains(&id.name()) || self.descriptor_cache.contains(&id.name())
	}

	#[profiling::function]
	pub fn create_pending_images(&mut self, chain: &Chain) -> anyhow::Result<()> {
		use graphics::descriptor::update::*;
		let image_ids = self
			.image_cache
			.load_pending(chain, chain.signal_sender())?;
		for (image_id, image_name) in image_ids.into_iter() {
			let cached_image = &self.image_cache[&image_id];
			let descriptor_set = self.descriptor_cache.insert(
				image_id,
				image_name.map(|v| format!("UI.Image.{}", v)),
				chain.persistent_descriptor_pool(),
			)?;
			Queue::default()
				.with(Operation::Write(WriteOp {
					destination: Descriptor {
						set: descriptor_set.clone(),
						binding_index: 0,
						array_element: 0,
					},
					kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					object: ObjectKind::Image(vec![ImageKind {
						sampler: cached_image.sampler.clone(),
						view: cached_image.view.clone(),
						layout: flags::ImageLayout::ShaderReadOnlyOptimal,
					}]),
				}))
				.apply(&*chain.logical()?);
		}
		Ok(())
	}

	#[profiling::function]
	pub fn destroy_pipeline(&mut self) -> anyhow::Result<()> {
		self.drawable.destroy_pipeline()
	}

	#[profiling::function]
	pub fn create_pipeline(&mut self, chain: &Chain, subpass_index: usize) -> anyhow::Result<()> {
		use pipeline::state::*;
		self.drawable.create_pipeline(
			&chain.logical()?,
			vec![self.descriptor_cache.layout()],
			pipeline::Pipeline::builder()
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::from(*chain.extent()))
				.set_rasterization_state(
					Rasterization::default().set_cull_mode(flags::CullMode::NONE),
				)
				.set_color_blending(
					color_blend::ColorBlend::default()
						.add_attachment(color_blend::Attachment::default()),
				)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			chain.render_pass(),
			subpass_index,
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

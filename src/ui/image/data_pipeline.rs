use crate::{
	asset,
	graphics::{
		self, command, descriptor, flags, image_view, pipeline, sampler, structs, Drawable, Texture,
	},
	task,
	ui::{image, mesh},
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

struct Loaded {
	view: sync::Arc<image_view::View>,
	sampler: sync::Arc<sampler::Sampler>,
	descriptor_set: sync::Weak<descriptor::Set>,
}

pub struct DataPipeline {
	pending_images: HashMap<image::Id, graphics::CompiledTexture>,
	images: HashMap<image::Id, Loaded>,

	descriptor_layout: sync::Arc<descriptor::SetLayout>,
	drawable: Drawable,
}

impl DataPipeline {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		let descriptor_layout = sync::Arc::new(
			descriptor::SetLayout::builder()
				.with_binding(
					0,
					flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					1,
					flags::ShaderKind::Fragment,
				)
				.build(&render_chain.logical())?,
		);
		Ok(Self {
			descriptor_layout,
			drawable: Drawable::default(),
			images: HashMap::new(),
			pending_images: HashMap::new(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		self.drawable.add_shader(id)
	}

	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		self.drawable.create_shaders(render_chain)
	}

	pub fn add_pending(&mut self, id: &asset::Id, texture: Box<Texture>) -> VoidResult {
		self.pending_images
			.insert(id.to_str().to_owned(), texture.get_compiled().clone());
		Ok(())
	}

	#[profiling::function]
	pub fn create_pending_images(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let mut pending_gpu_signals = Vec::new();
		if !self.pending_images.is_empty() {
			let pending_images = self.pending_images.drain().collect::<Vec<_>>();
			for (id, pending) in pending_images.into_iter() {
				let (loaded, mut signals) = self.create_image(render_chain, &pending)?;
				pending_gpu_signals.append(&mut signals);
				self.images.insert(id, loaded);
			}
		}
		Ok(pending_gpu_signals)
	}

	fn create_image(
		&self,
		render_chain: &graphics::RenderChain,
		pending: &graphics::CompiledTexture,
	) -> utility::Result<(Loaded, Vec<sync::Arc<command::Semaphore>>)> {
		use graphics::{descriptor::*, image, structs::subresource, TaskGpuCopy};

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

		let descriptor_set = render_chain
			.persistent_descriptor_pool()
			.write()
			.unwrap()
			.allocate_descriptor_sets(&vec![self.descriptor_layout.clone()])?
			.pop()
			.unwrap();

		let loaded = Loaded {
			view,
			sampler,
			descriptor_set,
		};

		SetUpdate::default()
			.with(UpdateOperation::Write(WriteOp {
				destination: UpdateOperationSet {
					set: loaded.descriptor_set.clone(),
					binding_index: 0,
					array_element: 0,
				},
				kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
				object: ObjectKind::Image(vec![ImageKind {
					sampler: loaded.sampler.clone(),
					view: loaded.view.clone(),
					layout: flags::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				}]),
			}))
			.apply(&render_chain.logical());

		Ok((loaded, signals))
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
			Some(&self.descriptor_layout),
			pipeline::Info::default()
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(pipeline::ViewportState::from(resolution))
				.set_color_blending(
					pipeline::ColorBlendState::default()
						.add_attachment(pipeline::ColorBlendAttachment::default()),
				),
		)
	}

	pub fn has_image(&self, id: &image::Id) -> bool {
		self.images.contains_key(id) || self.pending_images.contains_key(id)
	}

	pub fn bind_pipeline(&self, buffer: &mut command::Buffer) {
		self.drawable.bind_pipeline(buffer)
	}

	pub fn bind_texture(&self, buffer: &mut command::Buffer, image_id: &image::Id) {
		assert!(self.images.contains_key(image_id));
		self.drawable.bind_descriptors(
			buffer,
			vec![&self.images[image_id].descriptor_set.upgrade().unwrap()],
		);
	}
}

use crate::{
	asset,
	graphics::{
		self, command, descriptor, flags, image_view, pipeline, sampler, shader, structs, Texture,
	},
	task, ui,
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

type Id = String;
struct Loaded {
	view: sync::Arc<image_view::View>,
	sampler: sync::Arc<sampler::Sampler>,
	descriptor_set: sync::Weak<descriptor::Set>,
}

pub struct ImagePipeline {
	pending_images: HashMap<Id, graphics::CompiledTexture>,
	images: HashMap<Id, Loaded>,

	pipeline: Option<pipeline::Pipeline>,
	pipeline_layout: Option<pipeline::Layout>,
	descriptor_layout: sync::Arc<descriptor::SetLayout>,

	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
}

impl ImagePipeline {
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
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
			pipeline_layout: None,
			pipeline: None,
			images: HashMap::new(),
			pending_images: HashMap::new(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();
		self.pending_shaders
			.insert(shader.kind(), shader.contents().clone());
		Ok(())
	}

	pub fn add_pending(&mut self, id: &asset::Id, texture: Box<Texture>) -> VoidResult {
		self.pending_images
			.insert(id.to_str().to_owned(), texture.get_compiled().clone());
		Ok(())
	}

	#[profiling::function]
	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		for (kind, binary) in self.pending_shaders.drain() {
			self.shaders.insert(
				kind,
				sync::Arc::new(shader::Module::create(
					render_chain.logical().clone(),
					shader::Info {
						kind: kind,
						entry_point: String::from("main"),
						bytes: binary,
					},
				)?),
			);
		}
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
	pub fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	#[profiling::function]
	pub fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		self.pipeline_layout = Some(
			pipeline::Layout::builder()
				.with_descriptors(&self.descriptor_layout)
				.build(render_chain.logical().clone())?,
		);
		self.pipeline = Some(
			pipeline::Info::default()
				.add_shader(sync::Arc::downgrade(
					self.shaders.get(&flags::ShaderKind::Vertex).unwrap(),
				))
				.add_shader(sync::Arc::downgrade(
					self.shaders.get(&flags::ShaderKind::Fragment).unwrap(),
				))
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<ui::mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(
					pipeline::ViewportState::default()
						.add_viewport(graphics::utility::Viewport::default().set_size(resolution))
						.add_scissor(graphics::utility::Scissor::default().set_size(resolution)),
				)
				.set_rasterization_state(pipeline::RasterizationState::default())
				.set_color_blending(pipeline::ColorBlendState::default().add_attachment(
					pipeline::ColorBlendAttachment {
						color_flags: flags::ColorComponent::R
							| flags::ColorComponent::G | flags::ColorComponent::B
							| flags::ColorComponent::A,
						blend: Some(pipeline::Blend {
							color: SrcAlpha * New + (One - SrcAlpha) * Old,
							alpha: One * New + Zero * Old,
						}),
					},
				))
				.create_object(
					render_chain.logical().clone(),
					&self.pipeline_layout.as_ref().unwrap(),
					&render_chain.render_pass(),
				)?,
		);

		Ok(())
	}

	pub fn has_image(&self, id: &Id) -> bool {
		self.images.contains_key(id) || self.pending_images.contains_key(id)
	}

	pub fn bind_pipeline(&self, buffer: &mut command::Buffer) {
		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
	}

	pub fn bind_texture(&self, buffer: &mut command::Buffer, texture_id: &Id) {
		assert!(self.has_image(texture_id));
		buffer.bind_descriptors(
			flags::PipelineBindPoint::GRAPHICS,
			self.pipeline_layout.as_ref().unwrap(),
			0,
			vec![&self.images[texture_id].descriptor_set.upgrade().unwrap()],
		);
	}
}

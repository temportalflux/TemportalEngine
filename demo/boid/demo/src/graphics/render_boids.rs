use crate::{
	engine::{self, asset, math::nalgebra::Vector2, utility::AnyError, Application},
	graphics::{
		self, buffer, camera, command,
		descriptor::{self, layout::SetLayout},
		flags, image, image_view, pipeline, sampler, shader, structs,
		utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder, NamedObject},
		Instance, RenderChain, Vertex,
	},
	BoidDemo,
};
use std::sync::{Arc, RwLock, Weak};

#[derive(Clone)]
struct Frame {
	instance_count: usize,
	instance_buffer: Arc<buffer::Buffer>,
}

pub struct RenderBoids {
	frames: Vec<Frame>,
	active_instance_buffer: Arc<buffer::Buffer>,
	active_instance_count: usize,
	pending_gpu_signals: Vec<Arc<command::Semaphore>>,

	index_count: usize,
	index_buffer: buffer::Buffer,
	vertex_buffer: buffer::Buffer,

	image_descriptor_set: Weak<descriptor::Set>,
	image_descriptor_layout: Arc<SetLayout>,
	image_sampler: Arc<sampler::Sampler>,
	image_view: Arc<image_view::View>,

	camera_uniform: camera::Uniform,

	vert_shader: Arc<shader::Module>,
	frag_shader: Arc<shader::Module>,

	pipeline: Option<pipeline::Pipeline>,
	pipeline_layout: Option<pipeline::layout::Layout>,

	render_chain: Arc<RwLock<RenderChain>>,
}

impl RenderBoids {
	pub fn new(
		render_chain: &Arc<RwLock<RenderChain>>,
	) -> Result<Arc<RwLock<RenderBoids>>, AnyError> {
		let vert_shader = Arc::new(Self::load_shader(
			&render_chain.read().unwrap(),
			engine::asset::Id::new(BoidDemo::name(), "vertex"),
		)?);
		let frag_shader = Arc::new(Self::load_shader(
			&render_chain.read().unwrap(),
			engine::asset::Id::new(BoidDemo::name(), "fragment"),
		)?);

		let image =
			Self::create_boid_image(&render_chain.read().unwrap(), Self::load_boid_texture()?)?;
		let image_view = Arc::new(Self::create_image_view(
			&render_chain.read().unwrap(),
			image,
		)?);
		let image_sampler = Arc::new(
			sampler::Sampler::builder()
				.with_name("BoidModel.Image.Sampler")
				.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
				.with_max_anisotropy(Some(
					render_chain
						.read()
						.unwrap()
						.physical()
						.max_sampler_anisotropy(),
				))
				.build(&render_chain.read().unwrap().logical())?,
		);

		let image_descriptor_layout = Arc::new(
			SetLayout::builder()
				.with_binding(
					0,
					flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					1,
					flags::ShaderKind::Fragment,
				)
				.build(&render_chain.read().unwrap().logical())?,
		);

		let image_descriptor_set = render_chain
			.write()
			.unwrap()
			.persistent_descriptor_pool()
			.write()
			.unwrap()
			.allocate_descriptor_sets(&vec![image_descriptor_layout.clone()])?
			.pop()
			.unwrap();

		let (vertex_buffer, index_buffer, index_count) =
			Self::create_boid_model(&render_chain.read().unwrap())?;
		let active_instance_buffer = Arc::new(Self::create_instance_buffer(
			&render_chain.read().unwrap(),
			10,
		)?);

		let frames = vec![
			Frame {
				instance_buffer: active_instance_buffer.clone(),
				instance_count: 0,
			};
			render_chain.read().unwrap().frame_count()
		];

		let strong = Arc::new(RwLock::new(RenderBoids {
			render_chain: render_chain.clone(),
			pipeline_layout: None,
			pipeline: None,
			vert_shader,
			frag_shader,
			camera_uniform: camera::Uniform::new(
				"RenderBoids.Camera",
				&render_chain.read().unwrap(),
			)?,
			image_view,
			image_sampler,
			image_descriptor_layout,
			image_descriptor_set,
			vertex_buffer,
			index_buffer,
			index_count,
			active_instance_buffer,
			active_instance_count: 0,
			frames,
			pending_gpu_signals: Vec::new(),
		}));

		render_chain
			.write()
			.unwrap()
			.add_render_chain_element(None, &strong)?;

		Ok(strong)
	}

	fn load_shader(render_chain: &RenderChain, id: asset::Id) -> Result<shader::Module, AnyError> {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<engine::graphics::Shader>()
			.unwrap();

		Ok(shader::Module::create(
			render_chain.logical().clone(),
			shader::Info {
				name: Some(format!("RenderBoids.Shader.{:?}", shader.kind())),
				kind: shader.kind(),
				entry_point: String::from("main"),
				bytes: shader.contents().clone(),
			},
		)?)
	}

	fn load_boid_texture() -> Result<Box<graphics::Texture>, AnyError> {
		Ok(
			asset::Loader::load_sync(&engine::asset::Id::new(BoidDemo::name(), "boid"))?
				.downcast::<graphics::Texture>()
				.unwrap(),
		)
	}

	fn create_boid_image(
		render_chain: &RenderChain,
		texture: Box<graphics::Texture>,
	) -> Result<Arc<image::Image>, AnyError> {
		let image = Arc::new(
			graphics::image::Image::builder()
				.with_name("BoidModel.Image")
				.with_alloc(
					graphics::alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_format(flags::format::SRGB_8BIT)
				.with_size(structs::Extent3D {
					width: texture.size().x as u32,
					height: texture.size().y as u32,
					depth: 1,
				})
				.with_usage(flags::ImageUsage::TRANSFER_DST)
				.with_usage(flags::ImageUsage::SAMPLED)
				.build(&render_chain.allocator())?,
		);
		graphics::TaskGpuCopy::new(image.wrap_name(|v| format!("Create({})", v)), &render_chain)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&texture.binary()[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.end()?
			.wait_until_idle()?;
		Ok(image)
	}

	fn create_image_view(
		render_chain: &RenderChain,
		image: Arc<image::Image>,
	) -> Result<image_view::View, AnyError> {
		Ok(image_view::View::builder()
			.with_name("BoidModel.Image.View")
			.for_image(image.clone())
			.with_view_type(flags::ImageViewType::TYPE_2D)
			.with_range(
				structs::subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
			)
			.build(&render_chain.logical())?)
	}

	fn create_boid_model(
		render_chain: &RenderChain,
	) -> Result<(buffer::Buffer, buffer::Buffer, usize), AnyError> {
		let half_unit = 0.5;
		let vertices = vec![
			Vertex::default()
				.with_pos([-half_unit, -half_unit].into())
				.with_tex_coord([0.0, 0.0].into()),
			Vertex::default()
				.with_pos([half_unit, -half_unit].into())
				.with_tex_coord([1.0, 0.0].into()),
			Vertex::default()
				.with_pos([half_unit, half_unit].into())
				.with_tex_coord([1.0, 1.0].into()),
			Vertex::default()
				.with_pos([-half_unit, half_unit].into())
				.with_tex_coord([0.0, 1.0].into()),
		];
		let indices = vec![0, 1, 2, 2, 3, 0];

		let vertex_buffer = graphics::buffer::Buffer::builder()
			.with_name("BoidModel.VertexBuffer")
			.with_usage(flags::BufferUsage::VERTEX_BUFFER)
			.with_usage(flags::BufferUsage::TRANSFER_DST)
			.with_size_of(&vertices[..])
			.with_alloc(
				graphics::alloc::Builder::default()
					.with_usage(flags::MemoryUsage::GpuOnly)
					.requires(flags::MemoryProperty::DEVICE_LOCAL),
			)
			.with_sharing(flags::SharingMode::EXCLUSIVE)
			.build(&render_chain.allocator())?;

		graphics::TaskGpuCopy::new(
			vertex_buffer.wrap_name(|v| format!("Write({})", v)),
			&render_chain,
		)?
		.begin()?
		.stage(&vertices[..])?
		.copy_stage_to_buffer(&vertex_buffer)
		.end()?
		.wait_until_idle()?;

		let index_buffer = graphics::buffer::Buffer::builder()
			.with_name("BoidModel.IndexBuffer")
			.with_usage(flags::BufferUsage::INDEX_BUFFER)
			.with_index_type(Some(flags::IndexType::UINT32))
			.with_usage(flags::BufferUsage::TRANSFER_DST)
			.with_size_of(&indices[..])
			.with_alloc(
				graphics::alloc::Builder::default()
					.with_usage(flags::MemoryUsage::GpuOnly)
					.requires(flags::MemoryProperty::DEVICE_LOCAL),
			)
			.with_sharing(flags::SharingMode::EXCLUSIVE)
			.build(&render_chain.allocator())?;

		graphics::TaskGpuCopy::new(
			index_buffer.wrap_name(|v| format!("Write({})", v)),
			&render_chain,
		)?
		.begin()?
		.stage(&indices[..])?
		.copy_stage_to_buffer(&index_buffer)
		.end()?
		.wait_until_idle()?;

		Ok((vertex_buffer, index_buffer, indices.len()))
	}

	fn create_instance_buffer(
		render_chain: &RenderChain,
		instance_count: usize,
	) -> Result<buffer::Buffer, AnyError> {
		Ok(graphics::buffer::Buffer::builder()
			.with_name("RenderBoids.InstanceBuffer")
			.with_usage(flags::BufferUsage::VERTEX_BUFFER)
			.with_usage(flags::BufferUsage::TRANSFER_DST)
			.with_size(std::mem::size_of::<Instance>() * instance_count)
			.with_alloc(
				graphics::alloc::Builder::default()
					.with_usage(flags::MemoryUsage::GpuOnly)
					.requires(flags::MemoryProperty::DEVICE_LOCAL),
			)
			.with_sharing(flags::SharingMode::EXCLUSIVE)
			.build(&render_chain.allocator())?)
	}
}

impl graphics::RenderChainElement for RenderBoids {
	fn name(&self) -> &'static str {
		"render-boids"
	}

	fn initialize_with(
		&mut self,
		render_chain: &mut graphics::RenderChain,
	) -> Result<Vec<Arc<command::Semaphore>>, AnyError> {
		use graphics::descriptor::update::*;

		Queue::default()
			.with(Operation::Write(WriteOp {
				destination: Descriptor {
					set: self.image_descriptor_set.clone(),
					binding_index: 0,
					array_element: 0,
				},
				kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
				object: ObjectKind::Image(vec![ImageKind {
					view: self.image_view.clone(),
					sampler: self.image_sampler.clone(),
					layout: flags::ImageLayout::ShaderReadOnlyOptimal,
				}]),
			}))
			.apply(&render_chain.logical());

		self.camera_uniform.write_descriptor_sets(render_chain);

		Ok(Vec::new())
	}

	fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> Result<(), AnyError> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: &Vector2<f32>,
		subpass_id: &Option<String>,
	) -> Result<(), AnyError> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		use pipeline::state::*;
		self.pipeline_layout = Some(
			pipeline::layout::Layout::builder()
				.with_name("RenderBoids.PipelineLayout")
				.with_descriptors(self.camera_uniform.layout())
				.with_descriptors(&self.image_descriptor_layout)
				.build(&render_chain.logical())?,
		);
		self.pipeline = Some(
			pipeline::Pipeline::builder()
				.with_name("RenderBoids.Pipeline")
				.add_shader(Arc::downgrade(&self.vert_shader))
				.add_shader(Arc::downgrade(&self.frag_shader))
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<Vertex>(0, flags::VertexInputRate::VERTEX)
						.with_object::<Instance>(1, flags::VertexInputRate::INSTANCE),
				)
				.set_viewport_state(
					Viewport::default()
						.add_viewport(graphics::utility::Viewport::default().set_size(
							structs::Extent2D {
								width: resolution.x as u32,
								height: resolution.y as u32,
							},
						))
						.add_scissor(graphics::utility::Scissor::default().set_size(
							structs::Extent2D {
								width: resolution.x as u32,
								height: resolution.y as u32,
							},
						)),
				)
				.set_rasterization_state(Rasterization::default())
				.set_color_blending(color_blend::ColorBlend::default().add_attachment(
					color_blend::Attachment {
						color_flags: flags::ColorComponent::R
							| flags::ColorComponent::G | flags::ColorComponent::B
							| flags::ColorComponent::A,
						blend: Some(color_blend::Blend {
							color: SrcAlpha * New + (One - SrcAlpha) * Old,
							alpha: One * New + Zero * Old,
						}),
					},
				))
				.build(
					render_chain.logical().clone(),
					&self.pipeline_layout.as_ref().unwrap(),
					&render_chain.render_pass(),
					subpass_id,
				)?,
		);

		Ok(())
	}

	fn take_gpu_signals(&mut self) -> Vec<Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}

	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> Result<(), AnyError> {
		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
		buffer.bind_descriptors(
			flags::PipelineBindPoint::GRAPHICS,
			self.pipeline_layout.as_ref().unwrap(),
			0,
			vec![
				&self.camera_uniform.get_set(frame).unwrap(),
				&self.image_descriptor_set.upgrade().unwrap(),
			],
		);
		buffer.bind_vertex_buffers(0, vec![&self.vertex_buffer], vec![0]);
		buffer.bind_vertex_buffers(1, vec![&self.frames[frame].instance_buffer], vec![0]);
		buffer.bind_index_buffer(&self.index_buffer, 0);
		buffer.draw(self.index_count, 0, self.frames[frame].instance_count, 0, 0);
		Ok(())
	}

	fn prerecord_update(
		&mut self,
		chain: &graphics::RenderChain,
		_buffer: &command::Buffer,
		frame: usize,
		resolution: &Vector2<f32>,
	) -> Result<bool, AnyError> {
		self.camera_uniform
			.write_camera(frame, resolution, &chain.camera())?;

		let mut requires_rerecording = false;
		if !Arc::ptr_eq(
			&self.frames[frame].instance_buffer,
			&self.active_instance_buffer,
		) {
			self.frames[frame].instance_buffer = self.active_instance_buffer.clone();
			requires_rerecording = true;
		}
		if self.frames[frame].instance_count != self.active_instance_count {
			self.frames[frame].instance_count = self.active_instance_count;
			requires_rerecording = true;
		}

		Ok(requires_rerecording)
	}
}

impl RenderBoids {
	pub fn set_instances(
		&mut self,
		instances: Vec<Instance>,
		expansion_step: usize,
	) -> Result<(), AnyError> {
		use graphics::alloc::Object;

		let supported_instance_count =
			self.active_instance_buffer.size() / std::mem::size_of::<Instance>();

		let mut chain = self.render_chain.write().unwrap();

		if supported_instance_count < instances.len() {
			log::info!(
				"Recreating instance buffer to support {} instances",
				supported_instance_count + expansion_step
			);
			self.active_instance_buffer = Arc::new(Self::create_instance_buffer(
				&chain,
				supported_instance_count + expansion_step,
			)?);
		}

		// Update buffer with data
		if instances.len() > 0 {
			let copy_task = graphics::TaskGpuCopy::new(
				self.active_instance_buffer
					.wrap_name(|v| format!("Write({})", v)),
				&mut chain,
			)?
			.begin()?
			.stage(&instances[..])?
			.copy_stage_to_buffer(&self.active_instance_buffer)
			.end()?;
			self.pending_gpu_signals
				.push(copy_task.gpu_signal_on_complete());
			copy_task.send_to(engine::task::sender());
		}

		if instances.len() != self.active_instance_count {
			self.active_instance_count = instances.len();
			chain.mark_commands_dirty();
		}

		Ok(())
	}
}

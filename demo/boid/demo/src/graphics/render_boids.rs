use crate::{
	engine::{self, asset, math::nalgebra::Vector2, Application},
	graphics::{
		self, alloc, buffer,
		camera::{self, DefaultCamera},
		chain::operation::RequiresRecording,
		chain::Operation,
		command,
		descriptor::{self, layout::SetLayout},
		device::logical,
		flags, image, image_view, pipeline,
		procedure::Phase,
		sampler, shader, structs,
		utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder, NamedObject},
		Chain, GpuOpContext, GpuOperationBuilder, Instance, Uniform, Vertex,
	},
	BoidDemo,
};
use anyhow::Result;
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

	index_count: usize,
	index_buffer: Arc<buffer::Buffer>,
	vertex_buffer: Arc<buffer::Buffer>,

	image_descriptor_set: Weak<descriptor::Set>,
	image_descriptor_layout: Arc<SetLayout>,
	image_sampler: Arc<sampler::Sampler>,
	image_view: Arc<image_view::View>,

	camera_uniform: Uniform,
	camera: DefaultCamera,

	vert_shader: Arc<shader::Module>,
	frag_shader: Arc<shader::Module>,

	pipeline: Option<Arc<pipeline::Pipeline>>,
	pipeline_layout: Option<pipeline::layout::Layout>,

	chain: Arc<RwLock<Chain>>,
}

impl RenderBoids {
	pub fn new(
		arc_chain: &Arc<RwLock<Chain>>,
		phase: &Arc<Phase>,
		wrapping_world_bounds_min: &Vector2<f32>,
		wrapping_world_bounds_max: &Vector2<f32>,
	) -> Result<Arc<RwLock<RenderBoids>>> {
		let strong = {
			let chain = arc_chain.read().unwrap();
			let vert_shader = Arc::new(Self::load_shader(
				engine::asset::Id::new(BoidDemo::name(), "vertex"),
				&chain.logical()?,
			)?);
			let frag_shader = Arc::new(Self::load_shader(
				engine::asset::Id::new(BoidDemo::name(), "fragment"),
				&chain.logical()?,
			)?);

			let image = Self::create_boid_image(Self::load_boid_texture()?, &*chain)?;
			let image_view = Arc::new(Self::create_image_view(image, &chain.logical()?)?);
			let image_sampler = Arc::new(
				sampler::Sampler::builder()
					.with_name("BoidModel.Image.Sampler")
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(chain.physical()?.max_sampler_anisotropy()))
					.build(&chain.logical()?)?,
			);

			let image_descriptor_layout = Arc::new(
				SetLayout::builder()
					.with_name("BoidModel.Image.DescriptorLayout")
					.with_binding(
						0,
						flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
						1,
						flags::ShaderKind::Fragment,
					)
					.build(&chain.logical()?)?,
			);

			let image_descriptor_set = chain
				.persistent_descriptor_pool()
				.write()
				.unwrap()
				.allocate_named_descriptor_sets(&vec![(
					image_descriptor_layout.clone(),
					Some("BoidModel.Image.Descriptor".to_string()),
				)])?
				.pop()
				.unwrap();

			let (vertex_buffer, index_buffer, index_count) = Self::create_boid_model(&*chain)?;
			let active_instance_buffer =
				Arc::new(Self::create_instance_buffer(&chain.allocator()?, 10)?);

			let frames = vec![
				Frame {
					instance_buffer: active_instance_buffer.clone(),
					instance_count: 0,
				};
				chain.view_count()
			];

			let camera_uniform = Uniform::new::<camera::ViewProjection, &str>(
				"RenderBoids.Camera",
				&chain.logical()?,
				&chain.allocator()?,
				chain.persistent_descriptor_pool(),
				chain.view_count(),
			)?;

			Arc::new(RwLock::new(RenderBoids {
				chain: arc_chain.clone(),
				pipeline_layout: None,
				pipeline: None,
				vert_shader,
				frag_shader,
				camera_uniform,
				camera: camera::DefaultCamera::default()
					.with_position([0.0, 0.0, -10.0].into())
					.with_projection(camera::Projection::Orthographic(
						camera::OrthographicBounds {
							x: [wrapping_world_bounds_min.x, wrapping_world_bounds_max.x].into(),
							y: [wrapping_world_bounds_min.y, wrapping_world_bounds_max.y].into(),
							z: [0.01, 100.0].into(),
						},
					)),
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
			}))
		};
		if let Ok(mut chain) = arc_chain.write() {
			chain.add_operation(phase, Arc::downgrade(&strong))?;
		}
		Ok(strong)
	}

	fn load_shader(id: asset::Id, logical: &Arc<logical::Device>) -> Result<shader::Module> {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();

		Ok(shader::Module::create(
			logical.clone(),
			shader::Info {
				name: Some(format!("RenderBoids.Shader.{:?}", shader.kind())),
				kind: shader.kind(),
				entry_point: String::from("main"),
				bytes: shader.contents().clone(),
			},
		)?)
	}

	fn load_boid_texture() -> Result<Box<graphics::Texture>> {
		Ok(
			asset::Loader::load_sync(&engine::asset::Id::new(BoidDemo::name(), "boid"))?
				.downcast::<graphics::Texture>()
				.unwrap(),
		)
	}

	fn create_boid_image(
		texture: Box<graphics::Texture>,
		context: &impl GpuOpContext,
	) -> Result<Arc<image::Image>> {
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
				.build(&context.object_allocator()?)?,
		);
		let _ = engine::task::current().block_on(
			GpuOperationBuilder::new(image.wrap_name(|v| format!("Create({})", v)), context)?
				.begin()?
				.format_image_for_write(&image)
				.stage(&texture.binary()[..])?
				.copy_stage_to_image(&image)
				.format_image_for_read(&image)
				.end()?,
		);
		Ok(image)
	}

	fn create_image_view(
		image: Arc<image::Image>,
		logical: &Arc<logical::Device>,
	) -> Result<image_view::View> {
		Ok(image_view::View::builder()
			.with_name("BoidModel.Image.View")
			.for_image(image.clone())
			.with_view_type(flags::ImageViewType::TYPE_2D)
			.with_range(
				structs::subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
			)
			.build(logical)?)
	}

	fn create_boid_model(
		context: &impl GpuOpContext,
	) -> Result<(Arc<buffer::Buffer>, Arc<buffer::Buffer>, usize)> {
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

		let vertex_buffer = Arc::new(
			graphics::buffer::Buffer::builder()
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
				.build(&context.object_allocator()?)?,
		);

		let _ = engine::task::current().block_on(
			GpuOperationBuilder::new(
				vertex_buffer.wrap_name(|v| format!("Write({})", v)),
				context,
			)?
			.begin()?
			.stage(&vertices[..])?
			.copy_stage_to_buffer(&vertex_buffer)
			.end()?,
		);

		let index_buffer = Arc::new(
			graphics::buffer::Buffer::builder()
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
				.build(&context.object_allocator()?)?,
		);

		let _ = engine::task::current().block_on(
			GpuOperationBuilder::new(index_buffer.wrap_name(|v| format!("Write({})", v)), context)?
				.begin()?
				.stage(&indices[..])?
				.copy_stage_to_buffer(&index_buffer)
				.end()?,
		);

		Ok((vertex_buffer, index_buffer, indices.len()))
	}

	fn create_instance_buffer(
		allocator: &Arc<alloc::Allocator>,
		instance_count: usize,
	) -> Result<buffer::Buffer> {
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
			.build(allocator)?)
	}
}

impl Operation for RenderBoids {
	fn initialize(&mut self, chain: &Chain) -> anyhow::Result<()> {
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
			.apply(&*chain.logical()?);

		self.camera_uniform
			.write_descriptor_sets(&*chain.logical()?);
		Ok(())
	}

	fn construct(&mut self, chain: &Chain, subpass_index: usize) -> anyhow::Result<()> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		use pipeline::state::*;
		self.pipeline_layout = Some(
			pipeline::layout::Layout::builder()
				.with_name("RenderBoids.PipelineLayout")
				.with_descriptors(self.camera_uniform.layout())
				.with_descriptors(&self.image_descriptor_layout)
				.build(&chain.logical()?)?,
		);
		self.pipeline = Some(Arc::new(
			pipeline::Pipeline::builder()
				.with_name("RenderBoids.Pipeline")
				.add_shader(Arc::downgrade(&self.vert_shader))
				.add_shader(Arc::downgrade(&self.frag_shader))
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<Vertex>(0, flags::VertexInputRate::VERTEX)
						.with_object::<Instance>(1, flags::VertexInputRate::INSTANCE),
				)
				.set_viewport_state(Viewport::from(*chain.extent()))
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
					chain.logical()?,
					&self.pipeline_layout.as_ref().unwrap(),
					&chain.render_pass(),
					subpass_index,
				)?,
		));

		Ok(())
	}

	fn deconstruct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	fn prepare_for_submit(
		&mut self,
		chain: &Chain,
		frame_image: usize,
	) -> anyhow::Result<RequiresRecording> {
		self.camera_uniform.write_data(
			frame_image,
			&self.camera.as_uniform_matrix(&chain.resolution()),
		)?;

		let mut requires_rerecording = false;
		if !Arc::ptr_eq(
			&self.frames[frame_image].instance_buffer,
			&self.active_instance_buffer,
		) {
			self.frames[frame_image].instance_buffer = self.active_instance_buffer.clone();
			requires_rerecording = true;
		}
		if self.frames[frame_image].instance_count != self.active_instance_count {
			self.frames[frame_image].instance_count = self.active_instance_count;
			requires_rerecording = true;
		}

		Ok(match requires_rerecording {
			true => RequiresRecording::CurrentFrame,
			false => RequiresRecording::NotRequired,
		})
	}

	fn record(&mut self, buffer: &mut command::Buffer, buffer_index: usize) -> anyhow::Result<()> {
		use graphics::debug;

		buffer.begin_label("Draw:Boids", debug::LABEL_COLOR_DRAW);

		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
		buffer.bind_descriptors(
			flags::PipelineBindPoint::GRAPHICS,
			self.pipeline_layout.as_ref().unwrap(),
			0,
			vec![
				&self.camera_uniform.get_set(buffer_index).unwrap(),
				&self.image_descriptor_set.upgrade().unwrap(),
			],
		);
		buffer.bind_vertex_buffers(0, vec![&self.vertex_buffer], vec![0]);
		buffer.bind_vertex_buffers(1, vec![&self.frames[buffer_index].instance_buffer], vec![0]);
		buffer.bind_index_buffer(&self.index_buffer, 0);
		buffer.draw(
			self.index_count,
			0,
			self.frames[buffer_index].instance_count,
			0,
			0,
		);

		buffer.end_label();
		Ok(())
	}
}

impl RenderBoids {
	pub fn set_instances(&mut self, instances: Vec<Instance>, expansion_step: usize) -> Result<()> {
		use graphics::alloc::Object;

		let supported_instance_count =
			self.active_instance_buffer.size() / std::mem::size_of::<Instance>();

		let mut chain = self.chain.write().unwrap();

		if supported_instance_count < instances.len() {
			log::info!(
				"Recreating instance buffer to support {} instances",
				supported_instance_count + expansion_step
			);
			self.active_instance_buffer = Arc::new(Self::create_instance_buffer(
				&chain.allocator()?,
				supported_instance_count + expansion_step,
			)?);
		}

		// Update buffer with data
		if instances.len() > 0 {
			GpuOperationBuilder::new(
				self.active_instance_buffer
					.wrap_name(|v| format!("Write({})", v)),
				&*chain,
			)?
			.begin()?
			.stage(&instances[..])?
			.copy_stage_to_buffer(&self.active_instance_buffer)
			.send_signal_to(chain.signal_sender())?
			.end()?;
		}

		if instances.len() != self.active_instance_count {
			self.active_instance_count = instances.len();
			chain.mark_commands_dirty();
		}

		Ok(())
	}
}

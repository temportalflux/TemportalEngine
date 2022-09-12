use crate::channels::mpsc::Sender;
use crate::{
	asset,
	graphics::{
		self, buffer, camera,
		chain::{operation::RequiresRecording, Chain, Operation},
		command, flags, pipeline,
		types::{Vec3, Vec4},
		utility::NamedObject,
		vertex_object, Drawable, GpuOpContext, GpuOperationBuilder, Uniform,
	},
	math::nalgebra::{Point3, Vector3, Vector4},
	EngineSystem,
};
use anyhow::Result;
use std::sync::{Arc, RwLock};
use vulkan_rs::procedure::Phase;

pub enum DebugRenderPipeline {
	LineSegment,
}

#[vertex_object]
#[derive(Debug, Default)]
pub struct LineSegmentVertex {
	#[vertex_attribute([R, G, B], Bit32, SFloat)]
	position: Vec3,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	color: Vec4,
}

impl LineSegmentVertex {
	pub fn with_pos(mut self, pos: Vector3<f32>) -> Self {
		self.position = pos.into();
		self
	}
	pub fn with_color(mut self, color: Vector4<f32>) -> Self {
		self.color = color.into();
		self
	}
}

pub enum DebugDraw {
	LineSegment(Point, Point),
}

pub struct Point {
	pub position: Point3<f32>,
	pub color: Vector4<f32>,
}

pub struct DebugRender {
	pending_objects: Vec<DebugDraw>,
	camera_uniform: Uniform,
	frames: Vec<Frame>,
	line_drawable: Drawable,
}

struct Frame {
	index_order: Vec<std::ops::Range<usize>>,
	index_buffer: Arc<buffer::Buffer>,
	vertex_buffer: Arc<buffer::Buffer>,
}

impl DebugRender {
	pub fn new(chain: &Chain) -> Result<Self> {
		Ok(Self {
			line_drawable: Drawable::default().with_name("DebugRender.Line"),
			frames: Vec::new(),
			camera_uniform: Uniform::new::<camera::ViewProjection, &str>(
				"DebugRender.Camera",
				&chain.logical()?,
				&chain.allocator()?,
				chain.persistent_descriptor_pool(),
				chain.view_count(),
			)?,
			pending_objects: Vec::new(),
		})
	}

	pub fn create<F>(
		chain: &Arc<RwLock<Chain>>,
		phase: &Arc<Phase>,
		initializer: F,
	) -> Result<Arc<RwLock<Self>>>
	where
		F: Fn(Self) -> Result<Self>,
	{
		let mut chain_write = chain.write().unwrap();
		let inst = initializer(Self::new(&chain_write)?)?;
		let render = Arc::new(RwLock::new(inst));
		chain_write.add_operation(phase, Arc::downgrade(&render), None)?;
		Ok(render)
	}

	pub fn with_engine_shaders(mut self) -> Result<Self> {
		self.initialize_engine_shaders()?;
		Ok(self)
	}

	pub fn initialize_engine_shaders(&mut self) -> Result<()> {
		use crate::{Application, EngineApp};
		self.add_shader(
			DebugRenderPipeline::LineSegment,
			&EngineApp::get_asset_id("shaders/debug/line_segment/vertex"),
		)?;
		self.add_shader(
			DebugRenderPipeline::LineSegment,
			&EngineApp::get_asset_id("shaders/debug/line_segment/fragment"),
		)?;
		Ok(())
	}

	pub fn with_shader(mut self, key: DebugRenderPipeline, id: &asset::Id) -> Result<Self> {
		self.add_shader(key, id)?;
		Ok(self)
	}

	pub fn add_shader(&mut self, key: DebugRenderPipeline, id: &asset::Id) -> Result<()> {
		use DebugRenderPipeline::*;
		match key {
			LineSegment => self.line_drawable.add_shader(id),
		}
	}
}

impl DebugRender {
	pub fn clear(&mut self) {
		self.pending_objects.clear();
	}

	pub fn draw(&mut self, item: DebugDraw) {
		self.pending_objects.push(item);
	}

	pub fn draw_segment(&mut self, start: Point, end: Point) {
		self.draw(DebugDraw::LineSegment(start, end));
	}
}

impl EngineSystem for DebugRender {
	fn update(&mut self, _: std::time::Duration, _: bool) {
		self.clear();
	}
}

impl Operation for DebugRender {
	#[profiling::function]
	fn initialize(&mut self, chain: &Chain) -> anyhow::Result<()> {
		self.camera_uniform
			.write_descriptor_sets(&*chain.logical()?);
		self.line_drawable.create_shaders(&chain.logical()?)?;

		self.frames.clear();
		for i in 0..chain.view_count() {
			self.frames.push(Frame {
				vertex_buffer: buffer::Buffer::create_gpu(
					Some(format!("DebugRender.Frame{}.VertexBuffer", i)),
					&chain.allocator()?,
					flags::BufferUsage::VERTEX_BUFFER,
					std::mem::size_of::<LineSegmentVertex>() * 10,
					None,
				)?,
				index_buffer: buffer::Buffer::create_gpu(
					Some(format!("DebugRender.Frame{}.IndexBuffer", i)),
					&chain.allocator()?,
					flags::BufferUsage::INDEX_BUFFER,
					std::mem::size_of::<u32>() * 10,
					Some(flags::IndexType::UINT32),
				)?,
				index_order: Vec::new(),
			});
		}
		for frame in self.frames.iter_mut() {
			frame.write_buffer_data(chain, chain.signal_sender(), &self.pending_objects)?;
		}

		Ok(())
	}

	#[profiling::function]
	fn construct(&mut self, chain: &Chain, subpass_index: usize) -> anyhow::Result<()> {
		use pipeline::state::*;
		self.line_drawable.create_pipeline(
			&chain.logical()?,
			vec![self.camera_uniform.layout()],
			pipeline::Pipeline::builder()
				.with_topology(
					Topology::default().with_primitive(flags::PrimitiveTopology::LINE_LIST),
				)
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<LineSegmentVertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::from(*chain.extent()))
				.set_rasterization_state(
					Rasterization::default().set_cull_mode(flags::CullMode::NONE),
				)
				.set_color_blending(
					color_blend::ColorBlend::default()
						.add_attachment(color_blend::Attachment::default()),
				),
			chain.render_pass(),
			subpass_index,
		)?;
		Ok(())
	}

	#[profiling::function]
	fn deconstruct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		self.line_drawable.destroy_pipeline()?;
		Ok(())
	}

	fn prepare_for_submit(
		&mut self,
		chain: &Chain,
		frame_image: usize,
	) -> anyhow::Result<RequiresRecording> {
		self.camera_uniform.write_data(
			frame_image,
			&camera::DefaultCamera::default().as_uniform_matrix(&chain.resolution()),
		)?;

		self.frames[frame_image].write_buffer_data(
			chain,
			chain.signal_sender(),
			&self.pending_objects,
		)?;

		Ok(RequiresRecording::NotRequired)
	}

	fn record(&mut self, buffer: &mut command::Buffer, buffer_index: usize) -> anyhow::Result<()> {
		use graphics::debug;
		let frame_data = &self.frames[buffer_index];
		buffer.begin_label("Draw:Debug", debug::LABEL_COLOR_DRAW);
		self.line_drawable.bind_pipeline(buffer);
		self.line_drawable.bind_descriptors(
			buffer,
			vec![&self.camera_uniform.get_set(buffer_index).unwrap()],
		);
		buffer.bind_vertex_buffers(0, vec![&frame_data.vertex_buffer], vec![0]);
		buffer.bind_index_buffer(&frame_data.index_buffer, 0);
		for range in frame_data.index_order.iter() {
			buffer.draw(range.end - range.start, range.start, 1, 0, 0);
		}
		buffer.end_label();
		Ok(())
	}
}

impl Frame {
	fn write_buffer_data(
		&mut self,
		context: &impl GpuOpContext,
		signal_sender: &Sender<Arc<command::Semaphore>>,
		objects: &Vec<DebugDraw>,
	) -> anyhow::Result<()> {
		let mut vertices: Vec<LineSegmentVertex> = Vec::new();
		let mut indices: Vec<u32> = Vec::new();
		self.index_order.clear();
		for kind in objects {
			match kind {
				DebugDraw::LineSegment(start, end) => {
					let index_start = indices.len();

					indices.push(vertices.len() as u32);
					vertices.push(LineSegmentVertex {
						position: start.position.coords.into(),
						color: start.color.into(),
					});

					indices.push(vertices.len() as u32);
					vertices.push(LineSegmentVertex {
						position: end.position.coords.into(),
						color: end.color.into(),
					});

					self.index_order.push(index_start..indices.len());
				}
			}
		}

		let vbuff_size = std::mem::size_of::<LineSegmentVertex>() * vertices.len();
		let ibuff_size = std::mem::size_of::<u32>() * indices.len();

		if vbuff_size > 0 {
			if let Some(vbuf) = Arc::get_mut(&mut self.vertex_buffer) {
				vbuf.expand(vbuff_size)?;
			}
			GpuOperationBuilder::new(
				self.vertex_buffer.wrap_name(|v| format!("Write({})", v)),
				context,
			)?
			.begin()?
			.stage_any(vbuff_size, |mem| mem.write_slice(&vertices))?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.send_signal_to(signal_sender)?
			.end()?;
		}

		if ibuff_size > 0 {
			if let Some(ibuf) = Arc::get_mut(&mut self.index_buffer) {
				ibuf.expand(ibuff_size)?;
			}
			GpuOperationBuilder::new(
				self.index_buffer.wrap_name(|v| format!("Write({})", v)),
				context,
			)?
			.begin()?
			.stage_any(ibuff_size, |mem| mem.write_slice(&indices))?
			.copy_stage_to_buffer(&self.index_buffer)
			.send_signal_to(signal_sender)?
			.end()?;
		}

		Ok(())
	}
}

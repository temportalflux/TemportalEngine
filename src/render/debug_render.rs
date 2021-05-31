use crate::{
	asset,
	graphics::{self, buffer, camera, command, flags, pipeline, utility::offset_of, Drawable},
	math::Vector,
	task,
	utility::{self, AnyError, VoidResult},
};
use std::sync::{Arc, RwLock};

pub enum DebugRenderPipeline {
	LineSegment,
}

struct LineSegmentVertex {
	position: Vector<f32, 4>,
	color: Vector<f32, 4>,
}

impl pipeline::vertex::Object for LineSegmentVertex {
	fn attributes() -> Vec<pipeline::vertex::Attribute> {
		vec![
			pipeline::vertex::Attribute {
				offset: offset_of!(LineSegmentVertex, position),
				format: flags::Format::R32G32B32_SFLOAT,
			},
			pipeline::vertex::Attribute {
				offset: offset_of!(LineSegmentVertex, color),
				format: flags::Format::R32G32B32A32_SFLOAT,
			},
		]
	}
}

pub struct DebugRender {
	camera_uniform: camera::Uniform,
	frames: Vec<Frame>,
	line_drawable: Drawable,
}

struct Frame {
	index_count: usize,
	index_buffer: Arc<buffer::Buffer>,
	vertex_buffer: Arc<buffer::Buffer>,
}

impl DebugRender {
	pub fn new(chain: &graphics::RenderChain) -> Result<Self, AnyError> {
		Ok(Self {
			line_drawable: Drawable::default(),
			frames: Vec::new(),
			camera_uniform: camera::Uniform::new(chain)?,
		})
	}

	pub fn create<F>(
		chain: &Arc<RwLock<graphics::RenderChain>>,
		initializer: F,
	) -> Result<Arc<RwLock<Self>>, AnyError>
	where
		F: Fn(Self) -> Result<Self, AnyError>,
	{
		let mut chain_write = chain.write().unwrap();
		let inst = initializer(Self::new(&chain_write)?)?;
		let render = Arc::new(RwLock::new(inst));
		chain_write.add_render_chain_element(&render)?;
		Ok(render)
	}

	pub fn with_engine_shaders(mut self) -> Result<Self, AnyError> {
		self.initialize_engine_shaders()?;
		Ok(self)
	}

	pub fn initialize_engine_shaders(&mut self) -> VoidResult {
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

	pub fn with_shader(
		mut self,
		key: DebugRenderPipeline,
		id: &asset::Id,
	) -> Result<Self, AnyError> {
		self.add_shader(key, id)?;
		Ok(self)
	}

	pub fn add_shader(&mut self, key: DebugRenderPipeline, id: &asset::Id) -> VoidResult {
		use DebugRenderPipeline::*;
		match key {
			LineSegment => self.line_drawable.add_shader(id),
		}
	}
}

impl graphics::RenderChainElement for DebugRender {
	#[profiling::function]
	fn initialize_with(
		&mut self,
		chain: &mut graphics::RenderChain,
	) -> utility::Result<Vec<Arc<command::Semaphore>>> {
		let mut gpu_signals = Vec::new();

		self.camera_uniform.write_descriptor_sets(chain);
		self.line_drawable.create_shaders(chain)?;

		self.frames.clear();
		for _ in 0..chain.frame_count() {
			self.frames.push(Frame {
				vertex_buffer: buffer::Buffer::create_gpu(
					&chain.allocator(),
					flags::BufferUsage::VERTEX_BUFFER,
					std::mem::size_of::<LineSegmentVertex>() * 10,
				)?,
				index_buffer: buffer::Buffer::create_gpu(
					&chain.allocator(),
					flags::BufferUsage::INDEX_BUFFER,
					std::mem::size_of::<u32>() * 10,
				)?,
				index_count: 0,
			});
		}
		for frame in self.frames.iter() {
			let mut signals = frame.write_buffer_data(&chain)?;
			gpu_signals.append(&mut signals);
		}

		Ok(gpu_signals)
	}

	#[profiling::function]
	fn destroy_render_chain(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		self.line_drawable.destroy_pipeline(render_chain)?;
		Ok(())
	}

	#[profiling::function]
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: graphics::structs::Extent2D,
	) -> utility::Result<()> {
		self.line_drawable.create_pipeline(
			render_chain,
			vec![self.camera_uniform.layout()],
			pipeline::Info::default()
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<LineSegmentVertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(pipeline::ViewportState::from(resolution))
				.set_rasterization_state(
					pipeline::RasterizationState::default().set_cull_mode(flags::CullMode::NONE),
				)
				.set_color_blending(
					pipeline::ColorBlendState::default()
						.add_attachment(pipeline::ColorBlendAttachment::default()),
				),
		)
	}

	/// Update the data (like uniforms) for a given frame.
	#[profiling::function]
	fn prerecord_update(
		&mut self,
		chain: &graphics::RenderChain,
		_buffer: &command::Buffer,
		frame: usize,
		resolution: &Vector<u32, 2>,
	) -> utility::Result<bool> {
		self.camera_uniform
			.write_camera(frame, resolution.try_into().unwrap(), &chain.camera())?;
		Ok(false)
	}

	/// Record to the primary command buffer for a given frame
	#[profiling::function]
	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> utility::Result<()> {
		let frame_data = &self.frames[frame];
		self.line_drawable.bind_pipeline(buffer);
		self.line_drawable
			.bind_descriptors(buffer, vec![&self.camera_uniform.get_set(frame).unwrap()]);
		buffer.bind_vertex_buffers(0, vec![&frame_data.vertex_buffer], vec![0]);
		buffer.bind_index_buffer(&frame_data.index_buffer, 0);
		buffer.draw(frame_data.index_count, 0, 1, 0, 0);
		Ok(())
	}
}

impl Frame {
	fn write_buffer_data(
		&self,
		chain: &graphics::RenderChain,
	) -> utility::Result<Vec<Arc<command::Semaphore>>> {
		use graphics::alloc::Object;
		let mut gpu_signals = Vec::new();

		// TODO: pull this from some compiled list
		let data_size = self.vertex_buffer.size();
		let empty_data = vec![0_u8; data_size];

		//buffer.expand(std::mem::size_of::<T>() * data.len())?;
		graphics::TaskGpuCopy::new(&chain)?
			.begin()?
			.stage_any(data_size, |mem| mem.write_slice(&empty_data))?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		// TODO: pull this from some compiled list
		let data_size = self.index_buffer.size();
		let empty_data = vec![0_u8; data_size];

		//buffer.expand(std::mem::size_of::<T>() * data.len())?;
		graphics::TaskGpuCopy::new(&chain)?
			.begin()?
			.stage_any(data_size, |mem| mem.write_slice(&empty_data))?
			.copy_stage_to_buffer(&self.index_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		Ok(gpu_signals)
	}
}

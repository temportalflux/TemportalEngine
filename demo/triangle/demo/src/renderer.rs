use crate::{
	engine::{
		self, asset,
		graphics::{
			self, buffer, command, flags, pipeline, shader, structs,
			utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder, NamedObject},
			GpuOperationBuilder, RenderChain,
		},
		math::nalgebra::Vector2,
		utility::Result,
	},
	Vertex,
};
use std::sync::{Arc, RwLock};

pub struct Triangle {
	index_buffer: Option<Arc<buffer::Buffer>>,
	vertex_buffer: Option<Arc<buffer::Buffer>>,
	indices: Vec<u32>,
	vertices: Vec<Vertex>,
	vert_bytes: Vec<u8>,
	frag_bytes: Vec<u8>,
	vert_shader: Option<Arc<shader::Module>>,
	frag_shader: Option<Arc<shader::Module>>,
	pipeline: Option<Arc<pipeline::Pipeline>>,
	pipeline_layout: Option<pipeline::layout::Layout>,
}

impl Triangle {
	pub fn new(render_chain: &Arc<RwLock<RenderChain>>) -> Result<Arc<RwLock<Triangle>>> {
		let vert_bytes: Vec<u8>;
		let frag_bytes: Vec<u8>;
		{
			{
				let shader = asset::Loader::load_sync(&engine::asset::Id::new(
					"demo-triangle",
					"triangle_vert",
				))?
				.downcast::<engine::graphics::Shader>()
				.unwrap();
				vert_bytes = shader.contents().clone();
			}
			{
				let shader = asset::Loader::load_sync(&engine::asset::Id::new(
					"demo-triangle",
					"triangle_frag",
				))?
				.downcast::<engine::graphics::Shader>()
				.unwrap();
				frag_bytes = shader.contents().clone();
			}
		}

		let strong = Arc::new(RwLock::new(Triangle {
			pipeline_layout: None,
			pipeline: None,
			vert_bytes,
			frag_bytes,
			vert_shader: None,
			frag_shader: None,
			vertices: vec![
				Vertex::default()
					.with_pos([0.0, -0.5].into())
					.with_color([1.0, 0.0, 0.0, 1.0].into()),
				Vertex::default()
					.with_pos([0.5, 0.5].into())
					.with_color([0.0, 1.0, 0.0, 1.0].into()),
				Vertex::default()
					.with_pos([-0.5, 0.5].into())
					.with_color([0.0, 0.0, 1.0, 1.0].into()),
			],
			indices: vec![0, 1, 2],
			vertex_buffer: None,
			index_buffer: None,
		}));

		render_chain
			.write()
			.unwrap()
			.add_render_chain_element(None, &strong)?;
		Ok(strong)
	}
}

impl graphics::RenderChainElement for Triangle {
	fn name(&self) -> &'static str {
		"render-triangle"
	}

	fn initialize_with(
		&mut self,
		render_chain: &mut graphics::RenderChain,
	) -> Result<Vec<Arc<command::Semaphore>>> {
		self.vert_shader = Some(Arc::new(shader::Module::create(
			render_chain.logical().clone(),
			shader::Info {
				name: Some("Shader.Vertex".to_string()),
				kind: flags::ShaderKind::Vertex,
				entry_point: String::from("main"),
				bytes: self.vert_bytes.clone(),
			},
		)?));

		self.frag_shader = Some(Arc::new(shader::Module::create(
			render_chain.logical().clone(),
			shader::Info {
				name: Some("Shader.Fragment".to_string()),
				kind: flags::ShaderKind::Fragment,
				entry_point: String::from("main"),
				bytes: self.frag_bytes.clone(),
			},
		)?));

		self.vertex_buffer = Some(Arc::new(
			graphics::buffer::Buffer::builder()
				.with_name("VertexBuffer")
				.with_usage(flags::BufferUsage::VERTEX_BUFFER)
				.with_usage(flags::BufferUsage::TRANSFER_DST)
				.with_size_of(&self.vertices[..])
				.with_alloc(
					graphics::alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_sharing(flags::SharingMode::EXCLUSIVE)
				.build(&render_chain.allocator())?,
		));

		let vertex_buffer_copy_signal = {
			let copy_task = GpuOperationBuilder::new(
				self.vertex_buffer
					.as_ref()
					.unwrap()
					.wrap_name(|v| format!("Write({})", v)),
				&render_chain,
			)?
			.begin()?
			.stage(&self.vertices[..])?
			.copy_stage_to_buffer(&self.vertex_buffer.as_ref().unwrap());
			let signal = copy_task.gpu_signal_on_complete();
			copy_task.end()?;
			signal
		};

		self.index_buffer = Some(Arc::new(
			graphics::buffer::Buffer::builder()
				.with_name("IndexBuffer")
				.with_usage(flags::BufferUsage::INDEX_BUFFER)
				.with_index_type(Some(flags::IndexType::UINT32))
				.with_usage(flags::BufferUsage::TRANSFER_DST)
				.with_size_of(&self.indices[..])
				.with_alloc(
					graphics::alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_sharing(flags::SharingMode::EXCLUSIVE)
				.build(&render_chain.allocator())?,
		));

		let index_buffer_copy_signal = {
			let copy_task = GpuOperationBuilder::new(
				self.index_buffer
					.as_ref()
					.unwrap()
					.wrap_name(|v| format!("Write({})", v)),
				&render_chain,
			)?
			.begin()?
			.stage(&self.indices[..])?
			.copy_stage_to_buffer(&self.index_buffer.as_ref().unwrap());
			let signal = copy_task.gpu_signal_on_complete();
			copy_task.end()?;
			signal
		};

		Ok(vec![vertex_buffer_copy_signal, index_buffer_copy_signal])
	}

	fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: &Vector2<f32>,
		subpass_id: &Option<String>,
	) -> Result<()> {
		use pipeline::state::*;
		self.pipeline_layout = Some(
			pipeline::layout::Layout::builder()
				.with_name("PipelineLayout")
				.build(&render_chain.logical())?,
		);
		self.pipeline = Some(Arc::new(
			pipeline::Pipeline::builder()
				.with_name("Pipeline")
				.add_shader(Arc::downgrade(self.vert_shader.as_ref().unwrap()))
				.add_shader(Arc::downgrade(self.frag_shader.as_ref().unwrap()))
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<Vertex>(0, flags::VertexInputRate::VERTEX),
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
						blend: None,
					},
				))
				.build(
					render_chain.logical().clone(),
					&self.pipeline_layout.as_ref().unwrap(),
					&render_chain.render_pass(),
					subpass_id,
				)?,
		));

		Ok(())
	}

	fn record_to_buffer(&self, buffer: &mut command::Buffer, _: usize) -> Result<()> {
		use graphics::debug;
		buffer.begin_label("Draw:Triangle", debug::LABEL_COLOR_DRAW);
		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
		buffer.bind_vertex_buffers(0, vec![self.vertex_buffer.as_ref().unwrap()], vec![0]);
		buffer.bind_index_buffer(self.index_buffer.as_ref().unwrap(), 0);
		buffer.draw(self.indices.len(), 0, 1, 0, 0);
		buffer.end_label();
		Ok(())
	}
}

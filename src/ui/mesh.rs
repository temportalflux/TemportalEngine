use crate::{
	graphics::{
		alloc, buffer, command, flags, pipeline, utility::offset_of, RenderChain, TaskGpuCopy,
	},
	math::{vector, Vector},
	task, utility,
};
use raui::renderer::tesselate::prelude::*;
use std::sync;

pub struct Mesh {
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
}

pub struct Vertex {
	pos: Vector<f32, 4>,
	tex_coord: Vector<f32, 4>,
	color: Vector<f32, 4>,
}

impl pipeline::vertex::Object for Vertex {
	fn attributes() -> Vec<pipeline::vertex::Attribute> {
		vec![
			pipeline::vertex::Attribute {
				offset: offset_of!(Vertex, pos),
				format: flags::Format::R32G32_SFLOAT,
			},
			pipeline::vertex::Attribute {
				offset: offset_of!(Vertex, tex_coord),
				format: flags::Format::R32G32_SFLOAT,
			},
			pipeline::vertex::Attribute {
				offset: offset_of!(Vertex, color),
				format: flags::Format::R32G32B32A32_SFLOAT,
			},
		]
	}
}

impl Mesh {
	#[profiling::function]
	pub fn new(
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
	) -> utility::Result<Self> {
		Ok(Self {
			vertex_buffer: buffer::Buffer::create_gpu(
				allocator,
				flags::BufferUsage::VERTEX_BUFFER,
				Self::vertex_buffer_size_for(item_count),
			)?,
			index_buffer: buffer::Buffer::create_gpu(
				allocator,
				flags::BufferUsage::INDEX_BUFFER,
				Self::index_buffer_size_for(item_count),
			)?,
			index_count: 0,
		})
	}

	fn vertex_buffer_size_for(item_count: usize) -> usize {
		std::mem::size_of::<Vertex>() * item_count * 4
	}

	fn index_buffer_size_for(item_count: usize) -> usize {
		std::mem::size_of::<u32>() * item_count * 6
	}

	#[profiling::function]
	pub fn write(
		&mut self,
		tesselation: &Tesselation,
		render_chain: &RenderChain,
		resolution: &Vector<u32, 2>,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let resolution = vector![resolution.x() as f32, resolution.y() as f32];
		let vertices = tesselation
			.vertices
			.as_interleaved()
			.unwrap()
			.into_iter()
			.map(|(pos, tex_coord, color)| Vertex {
				pos: vector![pos.0 as f32, pos.1 as f32]
					.scale(resolution)
					.subvec::<4>(None),
				tex_coord: vector![tex_coord.0 as f32, tex_coord.1 as f32].subvec::<4>(None),
				color: [
					color.0 as f32,
					color.1 as f32,
					color.2 as f32,
					color.3 as f32,
				]
				.into(),
			})
			.collect::<Vec<_>>();
		let indices = &tesselation.indices;
		self.index_count = indices.len();

		sync::Arc::get_mut(&mut self.vertex_buffer)
			.unwrap()
			.expand(std::mem::size_of::<Vertex>() * vertices.len())?;
		sync::Arc::get_mut(&mut self.index_buffer)
			.unwrap()
			.expand(std::mem::size_of::<u32>() * indices.len())?;

		let mut gpu_signals = Vec::with_capacity(2);
		TaskGpuCopy::new(&render_chain)?
			.begin()?
			.stage(&vertices[..])?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		TaskGpuCopy::new(&render_chain)?
			.begin()?
			.stage(&vertices[..])?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		Ok(gpu_signals)
	}
}

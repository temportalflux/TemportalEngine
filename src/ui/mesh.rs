use crate::{
	graphics::{
		alloc, buffer, command, flags, pipeline::state::vertex, utility::offset_of, RenderChain,
		TaskGpuCopy,
	},
	math::nalgebra::{Vector2, Vector4},
	task, utility,
};
use raui::renderer::tesselate::prelude::*;
use std::sync;

pub struct Mesh {
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
}

#[derive(Debug)]
pub struct Vertex {
	pos: Vector4<f32>,
	tex_coord: Vector4<f32>,
	color: Vector4<f32>,
}

impl vertex::Object for Vertex {
	fn attributes() -> Vec<vertex::Attribute> {
		vec![
			vertex::Attribute {
				offset: offset_of!(Vertex, pos),
				format: flags::format::VEC2,
			},
			vertex::Attribute {
				offset: offset_of!(Vertex, tex_coord),
				format: flags::format::VEC2,
			},
			vertex::Attribute {
				offset: offset_of!(Vertex, color),
				format: flags::format::VEC4,
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
		resolution: &Vector2<f32>,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let vertices = tesselation
			.vertices
			.as_interleaved()
			.unwrap()
			.into_iter()
			.map(|interleaved| Vertex::from_interleaved(interleaved, resolution))
			.collect::<Vec<_>>();
		//log::debug!("{:?}", tesselation.vertices.as_interleaved().unwrap());
		//log::debug!("{:?}", vertices);
		//log::debug!("{:?}", tesselation.indices);
		let indices = tesselation
			.indices
			.iter()
			.map(|i| *i as u32)
			.collect::<Vec<_>>();
		self.index_count = indices.len();

		let mut gpu_signals = Vec::with_capacity(2);

		if !vertices.is_empty() {
			Self::write_buffer(
				sync::Arc::get_mut(&mut self.vertex_buffer).unwrap(),
				&vertices[..],
				render_chain,
				&mut gpu_signals,
			)?;
		}
		if !indices.is_empty() {
			Self::write_buffer(
				sync::Arc::get_mut(&mut self.index_buffer).unwrap(),
				&indices[..],
				render_chain,
				&mut gpu_signals,
			)?;
		}

		Ok(gpu_signals)
	}

	fn write_buffer<T: Sized>(
		buffer: &mut buffer::Buffer,
		data: &[T],
		render_chain: &RenderChain,
		signals: &mut Vec<sync::Arc<command::Semaphore>>,
	) -> utility::Result<()> {
		buffer.expand(std::mem::size_of::<T>() * data.len())?;
		TaskGpuCopy::new(&render_chain)?
			.begin()?
			.stage(data)?
			.copy_stage_to_buffer(&buffer)
			.end()?
			.add_signal_to(signals)
			.send_to(task::sender());
		Ok(())
	}

	pub fn bind_buffers(&self, buffer: &command::Buffer) {
		buffer.bind_vertex_buffers(0, vec![&self.vertex_buffer], vec![0]);
		buffer.bind_index_buffer(&self.index_buffer, 0);
	}
}

impl Vertex {
	fn from_interleaved(
		interleaved: &TesselationVerticeInterleaved,
		resolution: &Vector2<f32>,
	) -> Self {
		let TesselationVerticeInterleaved {
			position,
			tex_coord,
			color,
		} = interleaved;
		let pos: Vector2<f32> = [position.x as f32, position.y as f32].into();
		let pos = pos.component_div(resolution);
		Self {
			pos: [pos.x * 2.0 - 1.0, pos.y * 2.0 - 1.0, 0.0, 0.0].into(),
			tex_coord: [tex_coord.x, tex_coord.y, 0.0, 0.0].into(),
			color: [color.r, color.g, color.b, color.a].into(),
		}
	}
}

use crate::{
	graphics::{alloc, buffer, command, flags, pipeline::state::vertex, RenderChain, TaskGpuCopy},
	task, utility,
};
use std::sync;

pub struct Mesh<Index, Vertex>
where
	Vertex: vertex::Object,
{
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
	index_t: std::marker::PhantomData<Index>,
	vertex_t: std::marker::PhantomData<Vertex>,
}

impl<Index, Vertex> Mesh<Index, Vertex>
where
	Vertex: vertex::Object,
{
	#[profiling::function]
	pub fn new(
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
	) -> utility::Result<Self> {
		Ok(Self {
			vertex_t: std::marker::PhantomData,
			index_t: std::marker::PhantomData,
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
		std::mem::size_of::<Index>() * item_count * 6
	}

	#[profiling::function]
	pub fn write(
		&mut self,
		vertices: &Vec<Vertex>,
		indices: &Vec<Index>,
		render_chain: &RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
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

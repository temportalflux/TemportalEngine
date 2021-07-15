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
	pub fn new_mesh(
		name: String,
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
		index_type: flags::IndexType,
	) -> utility::Result<Self> {
		Self::new_internal(name, allocator, item_count, index_type)
	}
}

impl<Vertex> Mesh<u16, Vertex>
where
	Vertex: vertex::Object,
{
	pub fn new(
		name: String,
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
	) -> utility::Result<Self> {
		Self::new_internal(name, allocator, item_count, flags::IndexType::UINT16)
	}
}

impl<Vertex> Mesh<u32, Vertex>
where
	Vertex: vertex::Object,
{
	pub fn new(
		name: String,
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
	) -> utility::Result<Self> {
		Self::new_internal(name, allocator, item_count, flags::IndexType::UINT32)
	}
}

impl<Index, Vertex> Mesh<Index, Vertex>
where
	Vertex: vertex::Object,
{
	#[profiling::function]
	fn new_internal(
		name: String,
		allocator: &sync::Arc<alloc::Allocator>,
		item_count: usize,
		index_type: flags::IndexType,
	) -> utility::Result<Self> {
		Ok(Self {
			vertex_t: std::marker::PhantomData,
			index_t: std::marker::PhantomData,
			vertex_buffer: buffer::Buffer::create_gpu(
				Some(format!("{}.VertexBuffer", name)),
				allocator,
				flags::BufferUsage::VERTEX_BUFFER,
				Self::vertex_buffer_size_for(item_count),
				None,
			)?,
			index_buffer: buffer::Buffer::create_gpu(
				Some(format!("{}.IndexBuffer", name)),
				allocator,
				flags::BufferUsage::INDEX_BUFFER,
				Self::index_buffer_size_for(item_count),
				Some(index_type),
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
			.set_stage_target(&buffer)
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

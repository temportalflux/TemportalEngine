use crate::{
	graphics::{
		alloc, buffer, command, device::logical, flags, image, structs::subresource, RenderChain,
	},
	math::Vector,
	utility,
};
use std::sync;

pub struct TaskCopyImageToGpu {
	staging_buffer: Option<buffer::Buffer>,
	command_buffer: Option<command::Buffer>,
	command_pool: sync::Arc<command::Pool>,
	queue: sync::Arc<logical::Queue>,
	allocator: sync::Arc<alloc::Allocator>,
	device: sync::Arc<logical::Device>,
}

impl TaskCopyImageToGpu {
	pub fn new(render_chain: &RenderChain) -> utility::Result<TaskCopyImageToGpu> {
		let command_pool = render_chain.transient_command_pool();
		Ok(TaskCopyImageToGpu {
			device: render_chain.logical().clone(),
			allocator: render_chain.allocator().clone(),
			queue: render_chain.graphics_queue().clone(),
			command_pool: command_pool.clone(),
			command_buffer: command_pool
				.allocate_buffers(1, flags::CommandBufferLevel::PRIMARY)?
				.pop(),
			staging_buffer: None,
		})
	}

	fn cmd(&self) -> &command::Buffer {
		self.command_buffer.as_ref().unwrap()
	}

	pub fn begin(self) -> utility::Result<Self> {
		optick::event!();
		self.cmd()
			.begin(Some(flags::CommandBufferUsage::ONE_TIME_SUBMIT))?;
		Ok(self)
	}

	pub fn end(self) -> utility::Result<Self> {
		optick::event!();
		(self.cmd().end())?;

		(self.queue.submit(
			vec![command::SubmitInfo::default().add_buffer(&self.cmd())],
			None,
		))?;

		Ok(self)
	}

	pub fn wait_until_idle(self) -> utility::Result<()> {
		optick::event!();
		Ok(self.device.wait_until_idle()?)
	}

	pub fn format_image_for_write(self, image: &sync::Arc<image::Image>) -> Self {
		optick::event!();
		self.cmd().mark_pipeline_barrier(command::PipelineBarrier {
			src_stage: flags::PipelineStage::TOP_OF_PIPE,
			dst_stage: flags::PipelineStage::TRANSFER,
			kinds: vec![command::BarrierKind::Image(
				command::ImageBarrier::default()
					.prevents(flags::Access::TRANSFER_WRITE)
					.with_image(sync::Arc::downgrade(&image))
					.with_range(
						subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
					)
					.with_layout(
						flags::ImageLayout::UNDEFINED,
						flags::ImageLayout::TRANSFER_DST_OPTIMAL,
					),
			)],
		});
		self
	}

	pub fn format_image_for_read(self, image: &sync::Arc<image::Image>) -> Self {
		optick::event!();
		self.cmd().mark_pipeline_barrier(command::PipelineBarrier {
			src_stage: flags::PipelineStage::TRANSFER,
			dst_stage: flags::PipelineStage::FRAGMENT_SHADER,
			kinds: vec![command::BarrierKind::Image(
				command::ImageBarrier::default()
					.requires(flags::Access::TRANSFER_WRITE)
					.prevents(flags::Access::SHADER_READ)
					.with_image(sync::Arc::downgrade(&image))
					.with_range(
						subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
					)
					.with_layout(
						flags::ImageLayout::TRANSFER_DST_OPTIMAL,
						flags::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					),
			)],
		});
		self
	}

	fn staging_buffer(&self) -> &buffer::Buffer {
		self.staging_buffer.as_ref().unwrap()
	}

	pub fn stage<T: Sized>(mut self, data: &[T]) -> utility::Result<Self> {
		optick::event!();
		let buf_size = data.len() * std::mem::size_of::<T>();
		let buffer = buffer::Buffer::create_staging(buf_size, &self.allocator)?;
		{
			let mut mem = buffer.memory()?;
			let wrote_all = mem
				.write_slice(data)
				.map_err(|e| utility::Error::GraphicsBufferWrite(e))?;
			assert!(wrote_all);
		}
		self.staging_buffer = Some(buffer);
		Ok(self)
	}

	pub fn copy_stage_to_image(self, image: &sync::Arc<image::Image>) -> Self {
		optick::event!();
		self.cmd().copy_buffer_to_image(
			&self.staging_buffer(),
			&image,
			flags::ImageLayout::TRANSFER_DST_OPTIMAL,
			vec![command::CopyBufferToImage {
				buffer_offset: 0,
				layers: subresource::Layers::default().with_aspect(flags::ImageAspect::COLOR),
				offset: Vector::filled(0),
				size: image.image_size(),
			}],
		);
		self
	}

	pub fn copy_stage_to_buffer(self, buffer: &buffer::Buffer) -> Self {
		use alloc::Object;
		optick::event!();
		self.cmd().copy_buffer_to_buffer(
			&self.staging_buffer(),
			&buffer,
			vec![command::CopyBufferRange {
				start_in_src: 0,
				start_in_dst: 0,
				size: self.staging_buffer().size(),
			}],
		);
		self
	}
}

impl Drop for TaskCopyImageToGpu {
	fn drop(&mut self) {
		self.command_pool
			.free_buffers(vec![self.command_buffer.take().unwrap()]);
	}
}

use crate::{
	graphics::{
		alloc, buffer, command, device::logical, flags, image, structs::subresource, RenderChain,
	},
	math::Vector,
	utility,
};
use futures::future::Future;
use std::sync;

struct State {
	is_complete: bool,
	waker: Option<std::task::Waker>,
}

pub struct TaskGpuCopy {
	state: sync::Arc<sync::Mutex<State>>,
	gpu_signal_on_complete: sync::Arc<command::Semaphore>,
	cpu_signal_on_complete: sync::Arc<command::Fence>,

	staging_buffer: Option<buffer::Buffer>,
	command_buffer: Option<command::Buffer>,
	command_pool: sync::Arc<command::Pool>,
	queue: sync::Arc<logical::Queue>,
	allocator: sync::Arc<alloc::Allocator>,
	device: sync::Arc<logical::Device>,
}

impl TaskGpuCopy {
	pub fn new(render_chain: &RenderChain) -> utility::Result<Self> {
		let command_pool = render_chain.transient_command_pool();

		let state = sync::Arc::new(sync::Mutex::new(State {
			is_complete: false,
			waker: None,
		}));

		Ok(Self {
			device: render_chain.logical().clone(),
			allocator: render_chain.allocator().clone(),
			queue: render_chain.graphics_queue().clone(),
			command_pool: command_pool.clone(),
			command_buffer: command_pool
				.allocate_buffers(1, flags::CommandBufferLevel::PRIMARY)?
				.pop(),
			staging_buffer: None,
			cpu_signal_on_complete: sync::Arc::new(command::Fence::new(
				&render_chain.logical(),
				flags::FenceState::default(),
			)?),
			gpu_signal_on_complete: sync::Arc::new(command::Semaphore::new(
				&render_chain.logical(),
			)?),
			state,
		})
	}

	pub fn send_to(self, spawner: &sync::Arc<crate::task::Sender>) {
		spawner.spawn(self)
	}

	fn cmd(&self) -> &command::Buffer {
		self.command_buffer.as_ref().unwrap()
	}

	#[profiling::function]
	pub fn begin(self) -> utility::Result<Self> {
		self.cmd()
			.begin(Some(flags::CommandBufferUsage::ONE_TIME_SUBMIT), None)?;
		Ok(self)
	}

	#[profiling::function]
	pub fn end(self) -> utility::Result<Self> {
		self.cmd().end()?;

		self.queue.submit(
			vec![command::SubmitInfo::default()
				.signal_when_complete(&self.gpu_signal_on_complete)
				.add_buffer(&self.cmd())],
			Some(&self.cpu_signal_on_complete),
		)?;

		let thread_device = self.device.clone();
		let thread_cpu_signal = self.cpu_signal_on_complete.clone();
		let thread_state = self.state.clone();
		std::thread::spawn(move || {
			thread_device
				.wait_for(&thread_cpu_signal, u64::MAX)
				.unwrap();
			let mut state = thread_state.lock().unwrap();
			state.is_complete = true;
			if let Some(waker) = state.waker.take() {
				waker.wake();
			}
		});

		Ok(self)
	}

	pub fn cpu_signal_on_complete(&self) -> sync::Arc<command::Fence> {
		self.cpu_signal_on_complete.clone()
	}

	pub fn gpu_signal_on_complete(&self) -> sync::Arc<command::Semaphore> {
		self.gpu_signal_on_complete.clone()
	}

	pub fn add_signal_to(self, signals: &mut Vec<sync::Arc<command::Semaphore>>) -> Self {
		signals.push(self.gpu_signal_on_complete());
		self
	}

	#[profiling::function]
	pub fn wait_until_idle(self) -> utility::Result<()> {
		Ok(self
			.device
			.wait_for(&self.cpu_signal_on_complete, u64::MAX)?)
	}

	#[profiling::function]
	pub fn format_image_for_write(self, image: &sync::Arc<image::Image>) -> Self {
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

	#[profiling::function]
	pub fn format_image_for_read(self, image: &sync::Arc<image::Image>) -> Self {
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

	#[profiling::function]
	pub fn stage<T: Sized>(mut self, data: &[T]) -> utility::Result<Self> {
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

	#[profiling::function]
	pub fn copy_stage_to_image(self, image: &sync::Arc<image::Image>) -> Self {
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

	#[profiling::function]
	pub fn copy_stage_to_buffer(self, buffer: &buffer::Buffer) -> Self {
		use alloc::Object;
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

impl Drop for TaskGpuCopy {
	fn drop(&mut self) {
		self.command_pool
			.free_buffers(vec![self.command_buffer.take().unwrap()]);
	}
}

impl Future for TaskGpuCopy {
	type Output = ();
	fn poll(
		self: std::pin::Pin<&mut Self>,
		ctx: &mut std::task::Context<'_>,
	) -> std::task::Poll<Self::Output> {
		use std::task::Poll;
		let mut state = self.state.lock().unwrap();
		if !state.is_complete {
			state.waker = Some(ctx.waker().clone());
			Poll::Pending
		} else {
			Poll::Ready(())
		}
	}
}

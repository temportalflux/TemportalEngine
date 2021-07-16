use crate::{
	graphics::{
		alloc, buffer, command, device::logical, flags, image, structs::subresource, RenderChain,
	},
	utility,
};
use futures::future::Future;
use std::sync;

struct State {
	is_complete: bool,
	waker: Option<std::task::Waker>,
}

/// A copy from CPU to GPU operation that happens asynchronously.
pub struct TaskGpuCopy {
	/// Indicates if the task is complete and how to tell the futures package when the task wakes up.
	state: sync::Arc<sync::Mutex<State>>,
	/// The gpu signal (semaphore) that can be waited on to know when the operation is complete.
	gpu_signal_on_complete: sync::Arc<command::Semaphore>,
	/// The cpu signal (fence) that can be waited on to know when the operation is complete.
	cpu_signal_on_complete: sync::Arc<command::Fence>,
	/// The intermediate CPU -> GPU buffer for holding data.
	staging_buffer: Option<buffer::Buffer>,
	/// The command recording to run to copy data from CPU to GPU.
	command_buffer: Option<command::Buffer>,
	/// The pool used to create the command buffer.
	command_pool: sync::Arc<command::Pool>,
	/// The queue the command is executed on.
	queue: sync::Arc<logical::Queue>,
	/// The object allocator used to create the intermediate buffer.
	allocator: sync::Arc<alloc::Allocator>,
	/// The logical/virtual graphics device the command happens on.
	device: sync::Arc<logical::Device>,
	name: Option<String>,
}

impl TaskGpuCopy {
	pub fn new(name: Option<String>, render_chain: &RenderChain) -> utility::Result<Self> {
		let command_pool = render_chain.transient_command_pool();

		let state = sync::Arc::new(sync::Mutex::new(State {
			is_complete: false,
			waker: None,
		}));

		let task_name = name.as_ref().map(|v| format!("Task.{}", v));

		Ok(Self {
			device: render_chain.logical().clone(),
			allocator: render_chain.allocator().clone(),
			queue: render_chain.graphics_queue().clone(),
			command_pool: command_pool.clone(),
			command_buffer: command_pool
				.allocate_named_buffers(
					vec![task_name.as_ref().map(|v| format!("{}.Command", v))],
					flags::CommandBufferLevel::PRIMARY,
				)?
				.pop(),
			staging_buffer: None,
			cpu_signal_on_complete: sync::Arc::new(command::Fence::new(
				&render_chain.logical(),
				name.as_ref()
					.map(|v| format!("Task.{}.Signals.CPU.OnComplete", v)),
				flags::FenceState::default(),
			)?),
			gpu_signal_on_complete: sync::Arc::new(command::Semaphore::new(
				&render_chain.logical(),
				name.as_ref()
					.map(|v| format!("Task.{}.Signals.GPU.OnComplete", v)),
			)?),
			state,
			name: task_name,
		})
	}

	/// Sends the task to the engine task management,
	/// where it will run until the operation is complete,
	/// and then be dropped (thereby dropping all of its contents).
	///
	/// Can only be called after [`end`](TaskGpuCopy::end).
	pub fn send_to(self, spawner: &sync::Arc<crate::task::Sender>) {
		spawner.spawn(self)
	}

	fn cmd(&self) -> &command::Buffer {
		self.command_buffer.as_ref().unwrap()
	}

	/// Begins the copy operation command.
	/// The [`end`](TaskGpuCopy::end) MUST be called once complete.
	#[profiling::function]
	pub fn begin(self) -> utility::Result<Self> {
		if let Some(name) = self.name.as_ref() {
			self.queue
				.begin_label(name.clone(), [0.957, 0.855, 0.298, 1.0]); // #f4da4c
		}
		self.cmd()
			.begin(Some(flags::CommandBufferUsage::ONE_TIME_SUBMIT), None)?;
		Ok(self)
	}

	/// Ends the copy operation command and submits it to the GPU for processing.
	/// If sent to the task manager via [`sent_to`](TaskGpuCopy::send_to),
	/// the task will be woken up when the commands are complete.
	#[profiling::function]
	pub fn end(self) -> utility::Result<Self> {
		self.cmd().end()?;

		self.queue.submit(
			vec![command::SubmitInfo::default()
				.signal_when_complete(&self.gpu_signal_on_complete)
				.add_buffer(&self.cmd())],
			Some(&self.cpu_signal_on_complete),
		)?;
		if self.name.is_some() {
			self.queue.end_label();
		}

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

	/// Returns the [`cpu-signal (fence)`](command::Fence) which
	/// will be signalled when the operation is complete.
	pub fn cpu_signal_on_complete(&self) -> sync::Arc<command::Fence> {
		self.cpu_signal_on_complete.clone()
	}

	/// Returns the [`gpu-signal (semaphore)`](command::Semaphore) which
	/// will be signalled when the operation is complete.
	///
	/// If you wish to stall the thread until the operation
	/// is complete, see [`wait_until_idle`](TaskGpuCopy::wait_until_idle).
	pub fn gpu_signal_on_complete(&self) -> sync::Arc<command::Semaphore> {
		self.gpu_signal_on_complete.clone()
	}

	/// Adds the [`gpu-signal (semaphore)`](command::Semaphore) to the provided vec.
	/// Can be called any time before [`send_to`](TaskGpuCopy::send_to),
	/// but should not be called if using [`wait_until_idle`](TaskGpuCopy::wait_until_idle).
	pub fn add_signal_to(self, signals: &mut Vec<sync::Arc<command::Semaphore>>) -> Self {
		signals.push(self.gpu_signal_on_complete());
		self
	}

	/// Stalls the current thread until the [`cpu-signal (fence)`](command::Fence)
	/// has been signalled by the GPU to indicate that the operation is complete.
	///
	/// Cannot be called if using [`send_to`](TaskGpuCopy::send_to),
	/// as the task would be immediately complete once `wait_until_idle` is done.
	#[profiling::function]
	pub fn wait_until_idle(self) -> utility::Result<()> {
		Ok(self
			.device
			.wait_for(&self.cpu_signal_on_complete, u64::MAX)?)
	}

	/// Instructs the task to try to move an image from an
	/// [`undefined`](flags::ImageLayout::Undefined) layout to the
	/// [`transfer destination`](flags::ImageLayout::TransferDstOptimal) layout.
	/// This format prepares the image for writing by [`copy_stage_to_image`](TaskGpuCopy::copy_stage_to_image).
	///
	/// Can only be called after [`begin`](TaskGpuCopy::begin) and before [`end`](TaskGpuCopy::end).
	///
	/// Should be called before initializing image data via [`copy_stage_to_image`](TaskGpuCopy::copy_stage_to_image).
	#[profiling::function]
	pub fn format_image_for_write(self, image: &sync::Arc<image::Image>) -> Self {
		use command::barrier::{Image, Kind, Pipeline};
		self.cmd().mark_pipeline_barrier(
			Pipeline::new(
				flags::PipelineStage::TopOfPipe,
				flags::PipelineStage::Transfer,
			)
			.with(Kind::Image(
				Image::default()
					.prevents(flags::Access::TransferWrite)
					.with_image(sync::Arc::downgrade(&image))
					.with_range(
						subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
					)
					.with_layout(
						flags::ImageLayout::Undefined,
						flags::ImageLayout::TransferDstOptimal,
					),
			)),
		);
		self
	}

	/// Instructs the task to try to move an image from the
	/// [`transfer destination`](flags::ImageLayout::TransferDstOptimal) layout
	/// to the [`read only`](flags::ImageLayout::ShaderReadOnlyOptimal) format.
	///
	/// Can only be called after [`begin`](TaskGpuCopy::begin) and before [`end`](TaskGpuCopy::end).
	///
	/// Should be called after initializing image data via [`copy_stage_to_image`](TaskGpuCopy::copy_stage_to_image).
	#[profiling::function]
	pub fn format_image_for_read(self, image: &sync::Arc<image::Image>) -> Self {
		use command::barrier::{Image, Kind, Pipeline};
		self.cmd().mark_pipeline_barrier(
			Pipeline::new(
				flags::PipelineStage::Transfer,
				flags::PipelineStage::FragmentShader,
			)
			.with(Kind::Image(
				Image::default()
					.requires(flags::Access::TransferWrite)
					.prevents(flags::Access::ShaderRead)
					.with_image(sync::Arc::downgrade(&image))
					.with_range(
						subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
					)
					.with_layout(
						flags::ImageLayout::TransferDstOptimal,
						flags::ImageLayout::ShaderReadOnlyOptimal,
					),
			)),
		);
		self
	}

	fn staging_buffer(&self) -> &buffer::Buffer {
		self.staging_buffer.as_ref().unwrap()
	}

	/// Creates an intermediate CPU & GPU compatible buffer,
	/// and copies some data to it,
	/// so that said data can be copied to a GPU-only buffer or image.
	///
	/// If performing a buffer or image write/copy operation, this must be called before
	/// [`copy_stage_to_buffer`](TaskGpuCopy::copy_stage_to_buffer)
	/// or [`copy_stage_to_image`](TaskGpuCopy::copy_stage_to_image) respectively.
	#[profiling::function]
	pub fn stage<T: Sized>(self, data: &[T]) -> utility::Result<Self> {
		self.stage_any(data.len() * std::mem::size_of::<T>(), |mem| {
			mem.write_slice(data)
		})
	}

	#[profiling::function]
	pub fn stage_any<F>(mut self, memory_size: usize, write: F) -> utility::Result<Self>
	where
		F: Fn(&mut alloc::Memory) -> std::io::Result<bool>,
	{
		let buffer = buffer::Buffer::create_staging(
			self.name
				.as_ref()
				.map(|name| format!("{}.StagingBuffer", name)),
			&self.allocator,
			memory_size,
		)?;
		{
			let mut mem = buffer.memory()?;
			let wrote_all = write(&mut mem).map_err(|e| utility::Error::GraphicsBufferWrite(e))?;
			assert!(wrote_all);
		}
		self.staging_buffer = Some(buffer);
		Ok(self)
	}

	/// Copies the contents of the staging buffer created by [`stage`](TaskGpuCopy::stage),
	/// into the provided image.
	///
	/// Can only be called after [`begin`](TaskGpuCopy::begin) & [`stage`](TaskGpuCopy::stage)
	/// and before [`end`](TaskGpuCopy::end).
	#[profiling::function]
	pub fn copy_stage_to_image(self, image: &sync::Arc<image::Image>) -> Self {
		self.cmd().copy_buffer_to_image(
			&self.staging_buffer(),
			&image,
			flags::ImageLayout::TransferDstOptimal,
			vec![command::CopyBufferToImage {
				buffer_offset: 0,
				layers: subresource::Layers::default().with_aspect(flags::ImageAspect::COLOR),
				offset: Default::default(),
				size: image.image_size(),
			}],
		);
		self
	}

	/// Copies the contents of the staging buffer created by [`stage`](TaskGpuCopy::stage),
	/// into the provided buffer.
	///
	/// Can only be called after [`begin`](TaskGpuCopy::begin) & [`stage`](TaskGpuCopy::stage)
	/// and before [`end`](TaskGpuCopy::end).
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

use crate::channels::mpsc::Sender;

use crate::graphics::{
	alloc, buffer, command,
	device::{logical, physical},
	flags, image,
	structs::subresource,
};
use crate::task::{self};
use std::sync::{self, Arc};

pub trait GpuOpContext {
	fn physical_device(&self) -> anyhow::Result<Arc<physical::Device>>;
	fn logical_device(&self) -> anyhow::Result<Arc<logical::Device>>;
	fn object_allocator(&self) -> anyhow::Result<Arc<alloc::Allocator>>;
	fn logical_queue(&self) -> &Arc<logical::Queue>;
	fn task_command_pool(&self) -> &Arc<command::Pool>;
}

/// A copy from CPU to GPU operation that happens asynchronously.
pub struct GpuOperationBuilder {
	/// The gpu signal (semaphore) that can be waited on to know when the operation is complete.
	gpu_signal_on_complete: sync::Arc<command::Semaphore>,
	/// The cpu signal (fence) that can be waited on to know when the operation is complete.
	cpu_signal_on_complete: sync::Arc<command::Fence>,
	/// The intermediate CPU -> GPU buffer for holding data.
	staging_buffer: Option<Arc<buffer::Buffer>>,
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
	name: String,
}

impl GpuOperationBuilder {
	#[profiling::function]
	pub fn new<T>(name: String, context: &T) -> anyhow::Result<Self>
	where
		T: GpuOpContext,
	{
		let command_pool = context.task_command_pool();

		let task_name = format!("Task.{}", name);

		Ok(Self {
			device: context.logical_device()?.clone(),
			allocator: context.object_allocator()?.clone(),
			queue: context.logical_queue().clone(),
			command_pool: command_pool.clone(),
			command_buffer: command_pool
				.allocate_named_buffers(
					vec![format!("{}.Command", task_name)],
					flags::CommandBufferLevel::PRIMARY,
				)?
				.pop(),
			staging_buffer: None,
			cpu_signal_on_complete: sync::Arc::new(command::Fence::new(
				&context.logical_device()?,
				&format!("Task.{}.Signals.CPU.OnComplete", name),
				flags::FenceState::default(),
			)?),
			gpu_signal_on_complete: sync::Arc::new(command::Semaphore::new(
				&context.logical_device()?,
				&format!("Task.{}.Signals.GPU.OnComplete", name),
			)?),
			name: task_name,
		})
	}

	fn cmd(&self) -> &command::Buffer {
		self.command_buffer.as_ref().unwrap()
	}

	fn cmd_mut(&mut self) -> &mut command::Buffer {
		self.command_buffer.as_mut().unwrap()
	}

	/// Begins the copy operation command.
	/// The [`end`](GpuOperationBuilder::end) MUST be called once complete.
	#[profiling::function]
	pub fn begin(mut self) -> anyhow::Result<Self> {
		self.queue
			.begin_label(self.name.clone(), [0.957, 0.855, 0.298, 1.0]); // #f4da4c
		self.cmd_mut()
			.begin(Some(flags::CommandBufferUsage::ONE_TIME_SUBMIT), None)?;
		Ok(self)
	}

	/// Ends the copy operation command and submits it to the GPU for processing.
	#[profiling::function]
	pub fn end(self) -> anyhow::Result<task::JoinHandle<()>> {
		self.cmd().end()?;

		self.queue.submit(
			vec![command::SubmitInfo::default()
				.signal_when_complete(&self.gpu_signal_on_complete)
				.add_buffer(&self.cmd())],
			Some(&self.cpu_signal_on_complete),
		)?;
		self.queue.end_label();

		let arc_self = sync::Arc::new(self);
		let async_handle = task::spawn("gpu-operation".to_string(), async move {
			// Wait for the command to be complete
			arc_self
				.device
				.wait_for(vec![&arc_self.cpu_signal_on_complete], u64::MAX)?;
			// Send the arc to be dropped on the main thread because thats where the graphics objects need to be.
			task::send_to_main_thread(arc_self)?;
			Ok(())
		});

		Ok(async_handle)
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
	/// is complete, see [`wait_until_idle`](GpuOperationBuilder::wait_until_idle).
	pub fn gpu_signal_on_complete(&self) -> sync::Arc<command::Semaphore> {
		self.gpu_signal_on_complete.clone()
	}

	/// Adds the [`gpu-signal (semaphore)`](command::Semaphore) to the provided vec.
	/// Should not be called if using [`wait_until_idle`](GpuOperationBuilder::wait_until_idle).
	pub fn add_signal_to(self, signals: &mut Vec<sync::Arc<command::Semaphore>>) -> Self {
		signals.push(self.gpu_signal_on_complete());
		self
	}

	pub fn send_signal_to(self, sender: &Sender<Arc<command::Semaphore>>) -> anyhow::Result<Self> {
		sender.send(self.gpu_signal_on_complete())?;
		Ok(self)
	}

	/// Stalls the current thread until the [`cpu-signal (fence)`](command::Fence)
	/// has been signalled by the GPU to indicate that the operation is complete.
	#[profiling::function]
	pub fn wait_until_idle(self) -> anyhow::Result<()> {
		Ok(self
			.device
			.wait_for(vec![&self.cpu_signal_on_complete], u64::MAX)?)
	}

	/// Instructs the task to try to move an image from an
	/// [`undefined`](flags::ImageLayout::Undefined) layout to the
	/// [`transfer destination`](flags::ImageLayout::TransferDstOptimal) layout.
	/// This format prepares the image for writing by [`copy_stage_to_image`](GpuOperationBuilder::copy_stage_to_image).
	///
	/// Can only be called after [`begin`](GpuOperationBuilder::begin) and before [`end`](GpuOperationBuilder::end).
	///
	/// Should be called before initializing image data via [`copy_stage_to_image`](GpuOperationBuilder::copy_stage_to_image).
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
	/// Can only be called after [`begin`](GpuOperationBuilder::begin) and before [`end`](GpuOperationBuilder::end).
	///
	/// Should be called after initializing image data via [`copy_stage_to_image`](GpuOperationBuilder::copy_stage_to_image).
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

	#[profiling::function]
	pub fn format_depth_image(self, image: &sync::Arc<image::Image>) -> Self {
		use command::barrier::{Image, Kind, Pipeline};
		let mut subresource_range =
			subresource::Range::default().with_aspect(flags::ImageAspect::DEPTH);
		if flags::format::stencilable(image.format()) {
			subresource_range = subresource_range.with_aspect(flags::ImageAspect::STENCIL);
		}
		self.cmd().mark_pipeline_barrier(
			Pipeline::new(
				flags::PipelineStage::TopOfPipe,
				flags::PipelineStage::EarlyFragmentTests,
			)
			.with(Kind::Image(
				Image::default()
					.prevents(flags::Access::DepthStencilAttachmentRead)
					.prevents(flags::Access::DepthStencilAttachmentWrite)
					.with_image(sync::Arc::downgrade(&image))
					.with_range(subresource_range)
					.with_layout(
						flags::ImageLayout::Undefined,
						flags::ImageLayout::DepthStencilAttachmentOptimal,
					),
			)),
		);
		self
	}

	fn staging_buffer(&self) -> &Arc<buffer::Buffer> {
		self.staging_buffer.as_ref().unwrap()
	}

	/// Creates an intermediate CPU & GPU compatible buffer,
	/// and copies some data to it,
	/// so that said data can be copied to a GPU-only buffer or image.
	///
	/// If performing a buffer or image write/copy operation, this must be called before
	/// [`copy_stage_to_buffer`](GpuOperationBuilder::copy_stage_to_buffer)
	/// or [`copy_stage_to_image`](GpuOperationBuilder::copy_stage_to_image) respectively.
	#[profiling::function]
	pub fn stage<T: Sized>(self, data: &[T]) -> anyhow::Result<Self> {
		self.stage_any(data.len() * std::mem::size_of::<T>(), |mem| {
			mem.write_slice(data)
		})
	}

	#[profiling::function]
	pub fn stage_start(&mut self, memory_size: usize) -> anyhow::Result<()> {
		let buffer = buffer::Buffer::create_staging(
			format!("{}.StagingBuffer", self.name),
			&self.allocator,
			memory_size,
		)?;
		self.staging_buffer = Some(Arc::new(buffer));
		Ok(())
	}

	pub fn staging_memory(&mut self) -> anyhow::Result<alloc::Memory> {
		Ok(self.staging_buffer.as_ref().cloned().unwrap().memory()?)
	}

	#[profiling::function]
	pub fn stage_any<F>(mut self, memory_size: usize, write: F) -> anyhow::Result<Self>
	where
		F: Fn(&mut alloc::Memory) -> std::io::Result<bool>,
	{
		self.stage_start(memory_size)?;
		{
			let mut mem = self.staging_buffer.as_ref().cloned().unwrap().memory()?;
			let wrote_all = write(&mut mem)?;
			assert!(wrote_all);
		}
		Ok(self)
	}

	/// Copies the contents of the staging buffer created by [`stage`](GpuOperationBuilder::stage),
	/// into the provided image.
	///
	/// Can only be called after [`begin`](GpuOperationBuilder::begin) & [`stage`](GpuOperationBuilder::stage)
	/// and before [`end`](GpuOperationBuilder::end).
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

	#[profiling::function]
	pub fn copy_stage_to_image_regions(
		self,
		image: &sync::Arc<image::Image>,
		regions: Vec<command::CopyBufferToImage>,
	) -> Self {
		self.cmd().copy_buffer_to_image(
			&self.staging_buffer(),
			&image,
			flags::ImageLayout::TransferDstOptimal,
			regions,
		);
		self
	}

	/// Copies the contents of the staging buffer created by [`stage`](GpuOperationBuilder::stage),
	/// into the provided buffer.
	///
	/// Can only be called after [`begin`](GpuOperationBuilder::begin) & [`stage`](GpuOperationBuilder::stage)
	/// and before [`end`](GpuOperationBuilder::end).
	#[profiling::function]
	pub fn copy_stage_to_buffer(self, buffer: &buffer::Buffer) -> Self {
		let range = command::CopyBufferRange {
			start_in_src: 0,
			start_in_dst: 0,
			size: self.staging_buffer().size(),
		};
		self.copy_stage_to_buffer_ranges(&buffer, vec![range])
	}

	#[profiling::function]
	pub fn copy_stage_to_buffer_ranges(
		self,
		buffer: &buffer::Buffer,
		ranges: Vec<command::CopyBufferRange>,
	) -> Self {
		self.cmd()
			.copy_buffer_to_buffer(&self.staging_buffer(), &buffer, ranges);
		self
	}
}

impl Drop for GpuOperationBuilder {
	fn drop(&mut self) {
		self.command_pool
			.free_buffers(vec![self.command_buffer.take().unwrap()]);
	}
}

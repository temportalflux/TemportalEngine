use crate::channels::mpsc::{Receiver, Sender};
use crate::graphics::{
	alloc,
	chain::{
		operation::{ProcedureOperations, RequiresRecording},
		procedure::ProcedureConfig,
		ArcResolutionProvider, Operation,
	},
	command::{
		self,
		frame::{self, AttachedView},
	},
	debug, descriptor,
	device::{
		logical, physical,
		swapchain::{self, Swapchain, SwapchainBuilder},
	},
	flags,
	image_view::View,
	procedure::{Attachment, Phase, Procedure},
	renderpass::{self, RecordInstruction},
	resource, structs, task,
	utility::{BuildFromDevice, NameableBuilder},
};
use nalgebra::Vector2;
use std::sync::{Arc, RwLock, Weak};
use vulkan_rs::structs::Extent2D;

/// The core logic behind handling rendering data via a render pass to some result.
/// Chain extensions and drawable elements can be attached to the chain
/// to change how the chain renders and what it renders to.
pub struct Chain {
	pub(crate) physical: Weak<physical::Device>,
	pub(crate) logical: Weak<logical::Device>,
	pub(crate) allocator: Weak<alloc::Allocator>,
	pub(crate) graphics_queue: Arc<logical::Queue>,
	pub(crate) transient_command_pool: Arc<command::Pool>,
	pub(crate) persistent_descriptor_pool: Arc<RwLock<descriptor::Pool>>,
	pub(crate) swapchain_builder: Box<dyn SwapchainBuilder + 'static + Send + Sync>,
	pub(crate) resolution_provider: ArcResolutionProvider,
	pub(crate) incoming_signals: Receiver<Arc<command::Semaphore>>,
	pub(crate) outgoing_signals: Sender<Arc<command::Semaphore>>,

	pub(crate) record_instruction: RecordInstruction,
	pub(crate) frame_command_pool: Option<command::Pool>,
	pub(crate) command_buffers: Vec<command::Buffer>,
	pub(crate) pass: Option<renderpass::Pass>,
	pub(crate) swapchain: Option<Box<dyn Swapchain + 'static + Send + Sync>>,
	pub(crate) frame_image_views: Vec<Arc<View>>,
	pub(crate) procedure: Procedure,
	pub(crate) swapchain_attachment: Option<Arc<Attachment>>,
	pub(crate) framebuffers: Vec<Arc<frame::Buffer>>,

	/// [GPU signal] [per-frame]
	/// Signaled when the swapchain has acquired the view target.
	pub(crate) frame_signal_view_acquired: Vec<command::Semaphore>,
	/// [GPU signal] [per-frame]
	/// Signaled when the commands have finished executing, and the frame is now ready for presentation.
	pub(crate) frame_signal_render_finished: Vec<command::Semaphore>,
	/// [CPU signal] [per-frame]
	/// Signaled when the frame's view target is not currently being used.
	/// Signal is reset/off while the view is being written to or currently presented.
	pub(crate) frame_signal_view_available: Vec<command::Fence>,
	/// Mapping of a given [frame view](Chain::frame_image_views) to the index of the frame which is using it.
	pub(crate) view_in_flight_frame: Vec<Option<usize>>,

	pub(crate) resources: resource::Registry,
	pub(crate) operations: ProcedureOperations,

	pub(crate) requires_construction: bool,
	pub(crate) requires_recording_by_view: Vec<bool>,
	pub(crate) next_frame: usize,
}

// Accessors for data from ChainBuilder
impl Chain {
	/// Returns a empty builder defined by [`derive_builder`].
	pub fn builder() -> super::ChainBuilder {
		super::ChainBuilder::default()
	}

	/// Returns a pointer to the physical rendering device / GPU.
	pub fn physical(&self) -> Result<Arc<physical::Device>, NoPhysicalDevice> {
		self.physical.upgrade().ok_or(NoPhysicalDevice)
	}

	/// Returns a pointer to the logical device for the GPU.
	pub fn logical(&self) -> Result<Arc<logical::Device>, NoLogicalDevice> {
		self.logical.upgrade().ok_or(NoLogicalDevice)
	}

	/// Returns a pointer to the graphics object allocator (for creating buffers and images).
	pub fn allocator(&self) -> Result<Arc<alloc::Allocator>, NoAllocator> {
		self.allocator.upgrade().ok_or(NoAllocator)
	}

	/// Returns a pointer to the logical queue for submitted graphics commands.
	pub fn graphics_queue(&self) -> &Arc<logical::Queue> {
		&self.graphics_queue
	}

	/// Returns a pointer to the command pool that should be used for one time submit / transient commands.
	/// This command pool is not dropped until the render chain is dropped.
	pub fn transient_command_pool(&self) -> &Arc<command::Pool> {
		&self.transient_command_pool
	}

	/// Returns a mutex-pointer to the descriptor pool used for allocating all descriptor sets.
	/// This command pool is not dropped until the render chain is dropped.
	pub fn persistent_descriptor_pool(&self) -> &Arc<RwLock<descriptor::Pool>> {
		&self.persistent_descriptor_pool
	}

	pub fn signal_sender(&self) -> &Sender<Arc<command::Semaphore>> {
		&self.outgoing_signals
	}
}

impl task::GpuOpContext for Chain {
	fn physical_device(&self) -> anyhow::Result<Arc<physical::Device>> {
		Ok(self.physical()?)
	}

	fn logical_device(&self) -> anyhow::Result<Arc<logical::Device>> {
		Ok(self.logical()?)
	}

	fn object_allocator(&self) -> anyhow::Result<Arc<alloc::Allocator>> {
		Ok(self.allocator()?)
	}

	fn logical_queue(&self) -> &Arc<logical::Queue> {
		self.graphics_queue()
	}

	fn task_command_pool(&self) -> &Arc<command::Pool> {
		self.transient_command_pool()
	}
}

/// The weak physical device on the [`Chain`] has been dropped already.
#[derive(thiserror::Error, Debug)]
pub struct NoPhysicalDevice;
impl std::fmt::Display for NoPhysicalDevice {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Physical device has already been dropped.")
	}
}

/// The weak logical device on the [`Chain`] has been dropped already.
#[derive(thiserror::Error, Debug)]
pub struct NoLogicalDevice;
impl std::fmt::Display for NoLogicalDevice {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Logical device has already been dropped.")
	}
}

/// The weak allocator on the [`Chain`] has been dropped already.
#[derive(thiserror::Error, Debug)]
pub struct NoAllocator;
impl std::fmt::Display for NoAllocator {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Allocator has already been dropped.")
	}
}

impl Chain {
	/// Changes the procedure of the chain.
	/// This will cause all operations to be dropped immediately, and the render pass to be rebuilt on the next frame.
	pub fn set_procedure(&mut self, procedure: Procedure, swapchain_attachment: Arc<Attachment>) {
		self.operations.set_phase_count(procedure.num_phases());
		self.procedure = procedure;
		self.swapchain_attachment = Some(swapchain_attachment);
	}

	pub fn apply_procedure<T>(&mut self) -> anyhow::Result<<T as ProcedureConfig>::Phases>
	where
		T: ProcedureConfig,
	{
		T::apply_to(self)
	}

	pub fn add_operation<T>(
		&mut self,
		phase: &Arc<Phase>,
		operation: Weak<RwLock<T>>,
	) -> Result<(), PhaseNotInProcedure>
	where
		T: Operation + 'static + Send + Sync,
	{
		let phase_index = self.procedure.position(phase).ok_or(PhaseNotInProcedure)?;
		self.operations.insert(phase_index, operation);
		Ok(())
	}

	pub fn swapchain_image_format(&self) -> flags::format::Format {
		self.swapchain_builder.image_format()
	}

	pub fn swapchain(
		&self,
	) -> Result<&Box<dyn Swapchain + 'static + Send + Sync>, SwapchainNotConstructed> {
		self.swapchain.as_ref().ok_or(SwapchainNotConstructed)
	}

	pub fn resources(&self) -> &resource::Registry {
		&self.resources
	}

	pub fn resources_mut(&mut self) -> &mut resource::Registry {
		&mut self.resources
	}
}

impl Chain {
	pub fn is_constructed(&self) -> bool {
		self.swapchain.is_some()
	}

	pub fn view_count(&self) -> usize {
		self.swapchain_builder.image_count()
	}

	/// Constructs the resolution-dependent objects in the chain and drawable elements.
	/// Any pre-existing pipelines and other objects will be dropped,
	/// destroying old chain objects and creating new ones on attached elements.
	#[profiling::function]
	fn construct(&mut self, extent: structs::Extent2D) -> anyhow::Result<()> {
		let prefix = match self.is_constructed() {
			true => "re",
			false => "",
		};
		log::info!(
			target: "chain", // TODO: use chain name
			"{}constructing with resolution <{}, {}>",
			prefix,
			extent.width,
			extent.height
		);

		self.frame_signal_view_acquired.clear();
		self.frame_signal_render_finished.clear();
		self.frame_signal_view_available.clear();
		self.view_in_flight_frame.clear();

		self.framebuffers.clear();
		self.frame_image_views.clear();
		self.command_buffers.clear();
		self.frame_command_pool = None;

		if self.is_constructed() {
			self.operations.deconstruct(&self)?;
		}

		self.requires_construction = false;

		let logical = self.logical()?;
		// Only need to be updated if the procedure changes
		self.pass = Some(self.procedure.build(&logical)?);

		self.create_commands(self.view_count())?;

		self.update_swapchain_info(extent)?;
		self.swapchain = Some(self.swapchain_builder.build(self.swapchain.take())?);

		// Only need to be updated if the procedure changes
		self.record_instruction = self.create_record_instruction()?;

		self.frame_image_views = self.swapchain()?.create_image_views()?;

		// Update the resources and collect their attachments
		let attachments = {
			profiling::scope!("update-attachments");
			let mut attachments = Vec::with_capacity(self.procedure.attachments().len());

			{
				let attachment = self.swapchain_attachment.as_ref().unwrap();
				let weak = Arc::downgrade(attachment);
				attachments.push((weak, AttachedView::PerFrame(self.frame_image_views.clone())));
			}

			for resource in self.resources.iter() {
				if let Ok(mut resource) = resource.write() {
					resource.construct(&self)?;

					for pair in resource.get_attachments().into_iter() {
						attachments.push(pair);
					}
				}
			}

			assert_eq!(attachments.len(), self.procedure.attachments().len());
			attachments
		};

		self.framebuffers = {
			let mut builder = frame::Buffer::multi_builder()
				.with_name("RenderChain.Frames") // TODO: Derive from chain name
				.with_extent(extent)
				.with_sizes(
					self.frame_image_views.len(),
					self.procedure.attachments().len(),
				);

			for (attachment, view) in attachments.into_iter() {
				let index = match self.procedure.attachments().position(&attachment) {
					Some(index) => index,
					None => return Err(AttachmentNotInProcedure)?,
				};
				builder.attach(index, view);
			}

			builder.build(&self.logical()?, &self.render_pass())?
		};

		// TODO: Next up is creating all of the semaphors and fences
		// Then its onto recording command buffers and rendering frames

		let frame_count = self.max_frame_count();
		self.frame_signal_view_acquired = self.create_semaphores(
			"RenderChain.Signals.GPU.ViewAcquired", // TODO: Use chain name
			frame_count,
		)?;
		self.frame_signal_render_finished = self.create_semaphores(
			"RenderChain.Signals.GPU.RenderFinished", // TODO: Use chain name
			frame_count,
		)?;
		self.frame_signal_view_available = self.create_fences(
			"RenderChain.Signals.CPU.ViewAvailable", // TODO: Use chain name
			frame_count,
			flags::FenceState::SIGNALED,
		)?;
		self.view_in_flight_frame = vec![None; self.frame_image_views.len()];

		self.mark_commands_dirty();

		self.operations.construct(&self)?;

		Ok(())
	}

	fn create_commands(&mut self, buffer_count: usize) -> anyhow::Result<()> {
		let logical = self.logical()?;
		let command_pool = command::Pool::builder()
			.with_name("CommandPool.Frames") // TODO: Derive from chain name
			.with_queue_family_index(self.graphics_queue.index())
			.with_flag(flags::CommandPoolCreate::RESET_COMMAND_BUFFER)
			.build(&logical)?;
		self.command_buffers =
			command_pool.allocate_buffers(buffer_count, flags::CommandBufferLevel::PRIMARY)?;
		self.frame_command_pool = Some(command_pool);
		Ok(())
	}

	fn update_swapchain_info(&mut self, extent: structs::Extent2D) -> anyhow::Result<()> {
		self.swapchain_builder.set_image_extent(extent);

		let physical = self.physical()?;
		self.swapchain_builder
			.set_present_mode(physical.selected_present_mode);

		let surface_support = physical.query_surface_support();
		self.swapchain_builder
			.set_surface_transform(surface_support.current_transform());

		Ok(())
	}

	fn create_record_instruction(&self) -> Result<RecordInstruction, MissingClearValues> {
		let mut instruction = RecordInstruction::default();
		instruction.set_extent(*self.extent());
		for attachment in self.procedure.attachments().iter() {
			let attachment = attachment.upgrade().unwrap();
			if let Some(clear_value) = attachment.clear_value() {
				instruction.add_clear_value(*clear_value);
			} else if !instruction.clear_values.is_empty() {
				return Err(MissingClearValues);
			}
		}
		Ok(instruction)
	}

	fn create_semaphores(
		&self,
		name: &str,
		amount: usize,
	) -> anyhow::Result<Vec<command::Semaphore>> {
		let logical = self.logical()?;
		let mut vec = Vec::with_capacity(amount);
		for i in 0..amount {
			vec.push(command::Semaphore::new(
				&logical,
				Some(format!("{}.{}", name, i)),
			)?);
		}
		Ok(vec)
	}

	fn create_fences(
		&self,
		name: &str,
		amount: usize,
		default_state: flags::FenceState,
	) -> anyhow::Result<Vec<command::Fence>> {
		let logical = self.logical()?;
		let mut vec = Vec::with_capacity(amount);
		for i in 0..amount {
			vec.push(command::Fence::new(
				&logical,
				Some(format!("{}.{}", name, i)),
				default_state,
			)?);
		}
		Ok(vec)
	}

	pub fn render_pass(&self) -> &renderpass::Pass {
		self.pass.as_ref().unwrap()
	}

	pub fn extent(&self) -> &Extent2D {
		self.swapchain_builder.image_extent()
	}

	pub fn resolution(&self) -> Vector2<f32> {
		let &Extent2D { width, height } = self.extent();
		Vector2::new(width as f32, height as f32)
	}

	/// Records commands to the command buffer for a given frame.
	#[profiling::function]
	fn record_commands(&mut self, buffer_index: usize) -> anyhow::Result<()> {
		let use_secondary_buffers = false;
		let cmd = &mut self.command_buffers[buffer_index];

		cmd.begin(None, None)?;
		cmd.start_render_pass(
			&self.framebuffers[buffer_index],
			self.pass.as_ref().unwrap(),
			self.record_instruction.clone(),
			use_secondary_buffers,
		);

		let phase_count = self.procedure.num_phases();
		for (subpass, phase) in self.procedure.iter().enumerate() {
			profiling::scope!(
				"record_phase",
				&format!("{}: {}\nframe: {}", subpass, phase.name(), buffer_index)
			);
			cmd.begin_label(
				format!("SubPass:{}", phase.name()),
				debug::LABEL_COLOR_SUB_PASS,
			);
			for operation in self.operations.iter(subpass) {
				if let Ok(mut locked) = operation.write() {
					locked.record(cmd, buffer_index)?;
				}
			}
			if subpass + 1 < phase_count {
				cmd.next_subpass(use_secondary_buffers);
			}
			cmd.end_label();
		}

		cmd.stop_render_pass();
		cmd.end()?;

		Ok(())
	}

	/// Marks all frames dirty, which results in the frames being re-recorded the next time they are rendered.
	pub fn mark_commands_dirty(&mut self) {
		self.requires_recording_by_view = vec![true; self.view_count()];
	}

	/// Returns the number of frames/images that can be in flight
	/// (being recorded to, being processed by the GPU commands, or being currently presented) at any given time.
	fn max_frame_count(&self) -> usize {
		std::cmp::max(self.view_count() - 1, 1)
	}

	#[profiling::function]
	pub fn render_frame(&mut self) -> anyhow::Result<()> {
		let logical = self.logical()?;

		// First and foremost, we need to make sure all of the operations are initialized.
		// If this is the first frame, then the chain hasn't been constructed yet, and that happens further down.
		{
			let removed_operations = self.operations.remove_dropped();
			let uninitialized_operations = self.operations.take_unintialized();
			let initialized_operations = self
				.operations
				.initialize_new_operations(uninitialized_operations, &self)?;
			if removed_operations || initialized_operations {
				self.mark_commands_dirty();
			}
		}

		if self.requires_construction || self.resolution_provider.has_changed() {
			profiling::scope!("reconstruct-chain");

			if !self.frame_signal_view_available.is_empty() {
				let all_frames_complete =
					self.frame_signal_view_available.iter().collect::<Vec<_>>();
				logical.wait_for(all_frames_complete, u64::MAX)?;
			}

			let extent = self.resolution_provider.query(&self.physical()?);
			if extent.width > 0 && extent.height > 0 {
				self.construct(extent)?;
			} else {
				return Ok(());
			}
		}

		self.operations.prepare_for_frame(&self)?;

		// Wait for our desired frame to be available.
		// This will unblock when the fence is signalled when submission is complete.
		// So a frame can only make it past this point when the command buffers for
		// the previous submission of the same frame have been fully processed.
		self.wait_until_frame_avilable(self.next_frame)?;

		let next_image_idx = match self.acquire_frame_view(self.next_frame)? {
			Some(idx) => idx,
			None => {
				self.requires_construction = true;
				return Ok(());
			}
		};

		// Now we also need to make sure we wait for the view target itself to be available.
		// If there is no frame using the view target, then the mapping will be None.
		// If there was a frame that used this target, we need to make sure that
		// the frame has finished processing its command buffers.
		if let Some(frame_idx) = self.view_in_flight_frame[next_image_idx] {
			self.wait_until_frame_avilable(frame_idx)?;
			self.view_in_flight_frame[next_image_idx] = None;
		}

		match self.operations.prepare_for_submit(&self, next_image_idx)? {
			RequiresRecording::NotRequired => {} // NO-OP
			RequiresRecording::CurrentFrame => {
				self.requires_recording_by_view[next_image_idx] = true;
			}
			RequiresRecording::AllFrames => {
				self.requires_recording_by_view.fill(true);
			}
		}

		if self.requires_recording_by_view[next_image_idx] {
			self.record_commands(next_image_idx)?;
			self.requires_recording_by_view[next_image_idx] = false;
		}

		// The view target is now being used by the frame we are about to submit.
		// It will remaining Some(idx) until the view is used next.
		self.view_in_flight_frame[next_image_idx] = Some(self.next_frame);

		// The frame is now in-flight, and will be signaled when the command buffer has been fully processed on the GPU.
		logical.reset_fences(&[&self.frame_signal_view_available[self.next_frame]])?;

		let required_semaphores = self.read_incoming_signals();
		self.submit_frame(self.next_frame, next_image_idx, required_semaphores)?;

		if self.present(self.next_frame, next_image_idx)? {
			self.requires_construction = true;
		}

		self.next_frame = (self.next_frame + 1) % self.max_frame_count();
		Ok(())
	}

	#[profiling::function]
	fn wait_until_frame_avilable(&self, frame: usize) -> anyhow::Result<()> {
		self.logical()?
			.wait_for(vec![&self.frame_signal_view_available[frame]], u64::MAX)?;
		Ok(())
	}

	/// Acquires a given view from the swapchain,
	/// signalling the [`frame_signal_view_acquired`] at `frame` when the acquisition has completed on the GPU.
	#[profiling::function]
	fn acquire_frame_view(&self, frame: usize) -> anyhow::Result<Option<usize>> {
		use swapchain::AcquiredImage;
		let acquisition_result = self.swapchain()?.acquire_next_image(
			u64::MAX,
			command::SyncObject::Semaphore(&self.frame_signal_view_acquired[frame]),
		);
		match acquisition_result {
			Ok(AcquiredImage::Available(index)) => Ok(Some(index)),
			Ok(AcquiredImage::Suboptimal(_index)) => Ok(None),
			Err(e) => match e.downcast_ref::<crate::graphics::utility::Error>() {
				Some(crate::graphics::utility::Error::RequiresRenderChainUpdate) => Ok(None),
				_ => Err(e),
			},
		}
	}

	#[profiling::function]
	fn read_incoming_signals(&self) -> Vec<Arc<command::Semaphore>> {
		let mut signals = Vec::new();
		while let Ok(signal) = self.incoming_signals.try_recv() {
			signals.push(signal);
		}
		signals
	}

	/// Submits the command buffer for the given index via the graphics queue.
	///
	/// Command buffer processing will wait for:
	/// - The view to be acquired on the GPU (as signaled by [`acquire_frame_view`]).
	/// - A set of provided GPU signals (semaphores).
	///
	/// Sets the state of a CPU and GPU signal when the commands have finished:
	/// - [`frame_signal_render_finished`] at `buffer_idx`, indicating that the render/commands have finished
	/// - [`frame_signal_view_available`] at `buffer_idx`, indicating that the view will no longer be written to
	#[profiling::function]
	fn submit_frame(
		&self,
		buffer_idx: usize,
		view_idx: usize,
		required_signals: Vec<Arc<command::Semaphore>>,
	) -> anyhow::Result<()> {
		self.graphics_queue
			.begin_label("Render", debug::LABEL_COLOR_RENDER_PASS);
		self.graphics_queue.submit(
			vec![command::SubmitInfo::default()
				// tell the gpu to wait until the image is available
				.wait_for(
					&self.frame_signal_view_acquired[buffer_idx],
					flags::PipelineStage::ColorAttachmentOutput,
				)
				.wait_for_semaphores(&required_signals)
				// denote which command buffer is being executed
				.add_buffer(&self.command_buffers[view_idx])
				// tell the gpu to signal a semaphore when the image is available again
				.signal_when_complete(&self.frame_signal_render_finished[buffer_idx])],
			Some(&self.frame_signal_view_available[buffer_idx]),
		)?;
		self.graphics_queue.end_label();
		Ok(())
	}

	#[profiling::function]
	fn present(&self, buffer_idx: usize, view_idx: usize) -> anyhow::Result<bool> {
		let khr_swapchain = match self.swapchain()?.as_khr() {
			Some(khr) => khr,
			None => return Ok(false),
		};

		self.graphics_queue
			.begin_label("Present", debug::LABEL_COLOR_PRESENT);
		let present_result = khr_swapchain.present(
			&self.graphics_queue,
			command::PresentInfo::default()
				.wait_for(&self.frame_signal_render_finished[buffer_idx])
				.add_image_index(view_idx as u32),
		);
		self.graphics_queue.end_label();
		profiling::finish_frame!();

		match present_result {
			Ok(is_suboptimal) => Ok(is_suboptimal),
			Err(e) => match e {
				crate::graphics::utility::Error::RequiresRenderChainUpdate => Ok(true),
				_ => Err(e)?,
			},
		}
	}
}

#[derive(thiserror::Error, Debug)]
pub struct MissingClearValues;
impl std::fmt::Display for MissingClearValues {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"If one clear value is supplied, then all attachments must have clear values"
		)
	}
}

#[derive(thiserror::Error, Debug)]
pub struct AttachmentNotInProcedure;
impl std::fmt::Display for AttachmentNotInProcedure {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"Detected attachment in a resource, but the attachment does not exist in the procedure."
		)
	}
}

#[derive(thiserror::Error, Debug)]
pub struct PhaseNotInProcedure;
impl std::fmt::Display for PhaseNotInProcedure {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "The provided phase is not in the current procedure.")
	}
}

#[derive(thiserror::Error, Debug)]
pub struct SwapchainNotConstructed;
impl std::fmt::Display for SwapchainNotConstructed {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "The swapchain has not been constructed.")
	}
}

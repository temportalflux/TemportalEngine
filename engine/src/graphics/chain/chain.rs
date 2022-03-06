use crate::graphics::{
	alloc,
	chain::operation::{ProcedureOperations, WeakOperation},
	command::{self, frame},
	descriptor,
	device::{
		logical, physical,
		swapchain::{Swapchain, SwapchainBuilder},
	},
	flags,
	image_view::View,
	procedure::{Phase, Procedure},
	renderpass::{self, RecordInstruction},
	resource, structs,
	utility::{BuildFromDevice, NameableBuilder},
};
use std::{
	collections::HashMap,
	sync::{Arc, RwLock, Weak},
};

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
	pub(crate) swapchain_builder: Box<dyn SwapchainBuilder + 'static>,

	pub(crate) record_instruction: RecordInstruction,
	pub(crate) frame_command_pool: Option<command::Pool>,
	pub(crate) command_buffers: Vec<command::Buffer>,
	pub(crate) pass: Option<renderpass::Pass>,
	pub(crate) swapchain: Option<Box<dyn Swapchain + 'static>>,
	pub(crate) frame_image_views: Vec<Arc<View>>,
	pub(crate) procedure: Procedure,
	pub(crate) framebuffers: Vec<Arc<frame::Buffer>>,

	pub(crate) resources: resource::Registry,
	pub(crate) operations: ProcedureOperations,
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
	pub fn set_procedure(&mut self, procedure: Procedure) {
		self.operations.set_phase_count(procedure.num_phases());
		self.procedure = procedure;
	}

	pub fn add_operation(
		&mut self,
		phase: &Arc<Phase>,
		operation: WeakOperation,
	) -> Result<(), PhaseNotInProcedure> {
		let phase_index = self.procedure.position(phase).ok_or(PhaseNotInProcedure)?;
		self.operations.insert(phase_index, operation);
		Ok(())
	}

	pub fn swapchain_image_format(&self) -> flags::format::Format {
		self.swapchain_builder.image_format()
	}

	pub fn swapchain(&self) -> &Box<dyn Swapchain> {
		self.swapchain.as_ref().unwrap()
	}

	pub fn resources(&self) -> &resource::Registry {
		&self.resources
	}

	pub fn resources_mut(&mut self) -> &mut resource::Registry {
		&mut self.resources
	}
}

impl Chain {
	/// Constructs the resolution-dependent objects in the chain and drawable elements.
	/// Any pre-existing pipelines and other objects will be dropped,
	/// destroying old chain objects and creating new ones on attached elements.
	#[profiling::function]
	fn construct(&mut self, extent: structs::Extent2D) -> anyhow::Result<()> {
		self.framebuffers.clear();
		self.frame_image_views.clear();
		self.command_buffers.clear();
		self.frame_command_pool = None;

		for operation in self.operations.iter_all() {
			if let Ok(mut locked) = operation.write() {
				locked.deconstruct(&self)?;
			}
		}

		// These only need to be updated if the procedure changes
		{
			self.pass = Some(self.procedure.build(&self.logical()?)?);
			self.record_instruction = self.create_record_instruction()?;
		}

		let frame_count = self.swapchain_builder.image_count();
		self.create_commands(frame_count)?;

		self.update_swapchain_info(extent)?;
		self.swapchain = Some(self.swapchain_builder.build(self.swapchain.take())?);

		self.frame_image_views = self.swapchain().create_image_views()?;

		// Update the resources and collect their attachments
		let attachments = {
			profiling::scope!("update-attachments");
			let mut attachments = Vec::with_capacity(self.procedure.attachments().len());
			for resource in self.resources.iter() {
				if let Ok(mut resource) = resource.write() {
					resource.construct(&self)?;
					if let Some((attachment, view)) = resource.get_attachment_view() {
						if let Some(index) = self.procedure.attachments().position(&attachment) {
							attachments.push((index, view.clone()));
						} else {
							return Err(AttachmentNotInProcedure)?;
						}
					}
				}
			}
			assert_eq!(attachments.len(), self.procedure.attachments().len());
			attachments.sort_by(|&(a, _), &(b, _)| a.cmp(&b));
			attachments.into_iter().map(|(_, view)| view)
		};

		self.framebuffers = {
			let mut builder = frame::Buffer::multi_builder()
				.with_name("RenderChain.Frames") // TODO: Derive from chain name
				.with_extent(extent)
				.with_frame_count(self.frame_image_views.len())
				.attach_by_frame(self.frame_image_views.clone());
			for view in attachments {
				builder = builder.attach(view);
			}
			builder.build(&self.logical()?, &self.pass.as_ref().unwrap())?
		};

		// TODO: Next up is creating all of the semaphors and fences
		// Then its onto recording command buffers and rendering frames

		for operation in self.operations.iter_all() {
			if let Ok(mut locked) = operation.write() {
				locked.construct(&self)?;
			}
		}

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
		instruction.set_extent(*self.swapchain().image_extent());
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

	/// Records commands to the command buffer for a given frame.
	#[profiling::function]
	fn record_commands(&mut self, frame_index: usize) -> anyhow::Result<()> {
		Ok(())
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

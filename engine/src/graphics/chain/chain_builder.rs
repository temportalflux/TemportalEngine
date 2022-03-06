use crate::graphics::{
	alloc,
	chain::ArcResolutionProvider,
	command, descriptor,
	device::{logical, physical, swapchain::SwapchainBuilder},
	renderpass,
};
use std::sync::{Arc, RwLock};

#[derive(Default)]
pub struct ChainBuilder {
	physical_device: Option<Arc<physical::Device>>,
	logical_device: Option<Arc<logical::Device>>,
	allocator: Option<Arc<alloc::Allocator>>,
	graphics_queue: Option<Arc<logical::Queue>>,
	transient_command_pool: Option<Arc<command::Pool>>,
	persistent_descriptor_pool: Option<Arc<RwLock<descriptor::Pool>>>,
	swapchain_builder: Option<Box<dyn SwapchainBuilder + 'static>>,
	resolution_provider: Option<ArcResolutionProvider>,
}

impl ChainBuilder {
	pub fn with_physical_device(mut self, device: Arc<physical::Device>) -> Self {
		self.physical_device = Some(device);
		self
	}

	pub fn with_logical_device(mut self, device: Arc<logical::Device>) -> Self {
		self.logical_device = Some(device);
		self
	}

	pub fn with_allocator(mut self, allocator: Arc<alloc::Allocator>) -> Self {
		self.allocator = Some(allocator);
		self
	}

	pub fn with_graphics_queue(mut self, queue: Arc<logical::Queue>) -> Self {
		self.graphics_queue = Some(queue);
		self
	}

	pub fn with_transient_command_pool(mut self, pool: Arc<command::Pool>) -> Self {
		self.transient_command_pool = Some(pool);
		self
	}

	pub fn with_persistent_descriptor_pool(mut self, pool: Arc<RwLock<descriptor::Pool>>) -> Self {
		self.persistent_descriptor_pool = Some(pool);
		self
	}

	pub fn with_swapchain<T>(mut self, builder: T) -> Self
	where
		T: SwapchainBuilder + 'static,
	{
		self.swapchain_builder = Some(Box::new(builder));
		self
	}

	pub fn with_resolution_provider(mut self, provider: ArcResolutionProvider) -> Self {
		self.resolution_provider = Some(provider);
		self
	}

	pub fn build(self) -> Result<super::Chain, ChainBuilderError> {
		use ChainBuilderError::*;
		let (outgoing_signals, incoming_signals) = crossbeam_channel::unbounded();
		Ok(super::Chain {
			physical: Arc::downgrade(&self.physical_device.ok_or(NoPhysicalDevice)?),
			logical: Arc::downgrade(&self.logical_device.ok_or(NoLogicalDevice)?),
			allocator: Arc::downgrade(&self.allocator.ok_or(NoAllocator)?),
			graphics_queue: self.graphics_queue.ok_or(NoGraphicsQueue)?,
			transient_command_pool: self.transient_command_pool.ok_or(NoTransientCommandPool)?,
			persistent_descriptor_pool: self.persistent_descriptor_pool.ok_or(NoDescriptorPool)?,
			swapchain_builder: self.swapchain_builder.ok_or(NoSwapchainBuilder)?,
			resolution_provider: self.resolution_provider.unwrap(),
			incoming_signals,
			outgoing_signals,

			record_instruction: renderpass::RecordInstruction::default(),
			pass: None,
			swapchain: None,
			frame_image_views: Vec::new(),
			frame_command_pool: None,
			command_buffers: Vec::new(),
			procedure: Default::default(),
			swapchain_attachment: None,
			framebuffers: Vec::new(),
			frame_signal_view_acquired: Vec::new(),
			frame_signal_render_finished: Vec::new(),
			frame_signal_view_available: Vec::new(),
			view_in_flight_frame: Vec::new(),

			resources: Default::default(),
			operations: Default::default(),

			requires_construction: true,
			requires_recording_by_view: vec![],
			next_frame: 0,
		})
	}
}

#[derive(thiserror::Error, Debug)]
pub enum ChainBuilderError {
	#[error("Missing the physical_device parameter.")]
	NoPhysicalDevice,
	#[error("Missing the LogicalDevice parameter.")]
	NoLogicalDevice,
	#[error("Missing the allocator parameter.")]
	NoAllocator,
	#[error("Missing the graphics_queue parameter.")]
	NoGraphicsQueue,
	#[error("Missing the transient_command_pool parameter.")]
	NoTransientCommandPool,
	#[error("Missing the descriptor_pool parameter.")]
	NoDescriptorPool,
	#[error("Missing the swapchain builder parameter.")]
	NoSwapchainBuilder,
}

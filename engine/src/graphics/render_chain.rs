use crate::{
	graphics::{
		self,
		camera::Camera,
		command,
		device::{
			logical, physical,
			swapchain::{self, Swapchain},
		},
		flags, image, image_view, renderpass, structs,
		utility::{BuildFromDevice, NameableBuilder},
		Surface,
	},
	math::nalgebra::Vector2,
	utility::{self, AnyError},
};
use multimap::MultiMap;
use std::sync::{self, Arc, RwLock, Weak};

/// An object which contains data that needs to be updated when the render-chain is reconstructed
/// and records commands to one or more command buffers.
/// (i.e. something which contains a pipeline, and is therefore reliant on the resolution of the window).
pub trait RenderChainElement: Send + Sync {
	/// Returns some unique identifier that can be used when logging about the system
	fn name(&self) -> &'static str;

	/// Initializes the renderer before the first frame.
	/// Returns any semaphores which must complete before the first frame is submitted.
	fn initialize_with(
		&mut self,
		_render_chain: &mut RenderChain,
	) -> Result<Vec<Arc<command::Semaphore>>, AnyError> {
		Ok(vec![])
	}

	/// Creates any objects, like pipelines, which need to be created for a given swapchain (i.e. on window size change).
	/// Returns any semaphores which must complete before the next frame is submitted.
	/// Called any time the render chain is constructed
	/// (whereas `initialize_with` is only called once at the begining of an element's lifecycle).
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &RenderChain,
		resolution: &Vector2<f32>,
		subpass_id: &Option<String>,
	) -> Result<(), AnyError>;

	/// Performs any changes to data that need to happen before the frame begins to be processed
	/// (but after an uninitialized elements have been initialized).
	/// There is no frame-specific information provided to this function,
	/// see [`prerecord_update`](RenderChainElement::prerecord_update),
	/// if you need to make data changes for a specific frame.
	fn preframe_update(&mut self, _render_chain: &RenderChain) -> Result<(), AnyError> {
		Ok(())
	}

	/// Destroys any objects which are created during `on_render_chain_constructed`.
	/// The render chain may be reconstructed soon after this is called, or it may just be dropped entirely.
	/// This function is not garunteed to be called when the render chain is dropped.
	fn destroy_render_chain(&mut self, render_chain: &RenderChain) -> Result<(), AnyError>;

	/// Performs any tweaks that need to be made to data before the frame may be recorded.
	/// The frame may not be recorded after all elements have been processed,
	/// but if any element returns true from this function, the frame is marked as dirty and must be recorded.
	/// This is one way to enforce an immediate mode rendering (in which all frames are always rerecorded).
	fn prerecord_update(
		&mut self,
		_render_chain: &RenderChain,
		_buffer: &command::Buffer,
		_frame: usize,
		_resolution: &Vector2<f32>,
	) -> Result<bool, AnyError> {
		Ok(false)
	}

	/// Returns a list of gpu semaphores that need to be completed/signaled before the next frame can be submitted.
	/// This should modify any signals stored on the element,
	/// such that all future frames do not need to rely on the taken semaphores.
	fn take_gpu_signals(&mut self) -> Vec<Arc<command::Semaphore>> {
		Vec::new()
	}

	/// Records commands to the command buffer for a given frame.
	/// Only called if the frame has been marked as dirty, either by [`mark_commands_dirty`](RenderChain::mark_commands_dirty),
	/// or by any element returning `true` from [`prerecord_update`](RenderChainElement::prerecord_update).
	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> Result<(), AnyError>;
}

type ChainElement = Weak<RwLock<dyn graphics::RenderChainElement>>;
pub type ArcRenderChain = Arc<RwLock<RenderChain>>;

/// A general purpose renderer used for managing the recording of
/// command buffers, generating the swapchain and its framebuffers, etc.
///
/// Frames can be marked as dirty to force a re-recording the next time the frame is being preparred,
/// so render chains are optionally immediate mode.
///
/// Create a render chain from the display window via [`create_render_chain`](crate::window::Window::create_render_chain).
pub struct RenderChain {
	initialized_render_chain_elements:
		MultiMap</*render subpass*/ Option<String>, ChainElement>,
	pending_render_chain_elements: MultiMap</*render subpass*/ Option<String>, ChainElement>,

	current_frame: usize,
	images_in_flight: Vec<Option</*in_flight_fence index*/ usize>>,
	in_flight_fences: Vec<command::Fence>,
	render_finished_semaphores: Vec<command::Semaphore>,
	img_available_semaphores: Vec<command::Semaphore>,
	frame_command_buffer_requires_recording: Vec<bool>,

	command_buffers: Vec<command::Buffer>,
	frame_command_pool: Option<command::Pool>,
	frame_buffers: Vec<command::framebuffer::Framebuffer>,
	frame_image_views: Vec<image_view::View>,
	frame_images: Vec<sync::Arc<image::Image>>,
	swapchain: Option<Swapchain>,
	render_pass: Option<renderpass::Pass>,

	swapchain_info: swapchain::Builder,
	transient_command_pool: sync::Arc<command::Pool>,

	resolution: Vector2<f32>,
	is_dirty: bool,
	render_pass_info: renderpass::Info,
	render_pass_instruction: renderpass::RecordInstruction,

	frame_count: usize,
	camera: Camera,

	persistent_descriptor_pool: sync::Arc<sync::RwLock<graphics::descriptor::pool::Pool>>,
	surface: sync::Weak<Surface>,
	graphics_queue: sync::Arc<logical::Queue>,
	allocator: sync::Weak<graphics::alloc::Allocator>,
	logical: sync::Weak<logical::Device>,
	physical: sync::Weak<physical::Device>,
}

impl RenderChain {
	/// Constructs a render chain from vulkan device objects.
	///
	/// # Arguments
	///
	/// * `physical` - The GPU device
	/// * `logical` - The logical device for `physical`
	/// * `allocator` - The graphics device allocator for creating buffers and images
	/// * `graphics_queue` - The queue on which to submit graphics commands
	/// * `surface` - The window surface to which frames can be presented
	/// * `frame_count` - The number of frames to manage (2 for double buffer, 3 for ring buffer).
	/// 		This controls the number of swapchain images, framebuffers, and primary command buffers.
	pub fn new(
		physical: &sync::Arc<physical::Device>,
		logical: &sync::Arc<logical::Device>,
		allocator: &sync::Arc<graphics::alloc::Allocator>,
		graphics_queue: logical::Queue,
		surface: &sync::Arc<Surface>,
		frame_count: usize,
	) -> utility::Result<RenderChain> {
		let swapchain_info = Swapchain::builder()
			.with_name("Swapchain")
			.with_image_count(frame_count as u32)
			.with_image_format(flags::format::Format::B8G8R8A8_SRGB)
			.with_image_color_space(flags::ColorSpace::SRGB_NONLINEAR)
			.with_image_array_layer_count(1)
			.with_image_usage(flags::ImageUsageFlags::COLOR_ATTACHMENT)
			.with_image_sharing_mode(flags::SharingMode::EXCLUSIVE)
			.with_composite_alpha(flags::CompositeAlpha::OPAQUE)
			.with_is_clipped(true);
		let render_pass_instruction = renderpass::RecordInstruction::default();
		let resolution = physical.query_surface_support().image_extent();

		let transient_command_pool = sync::Arc::new(
			command::Pool::builder()
				.with_name("CommandPool.Transient")
				.with_queue_family_index(graphics_queue.index())
				.with_flag(flags::CommandPoolCreate::TRANSIENT)
				.build(&logical)?,
		);

		let persistent_descriptor_pool = sync::Arc::new(sync::RwLock::new(
			graphics::descriptor::pool::Pool::builder()
				.with_name("DescriptorPool.Persistent")
				.with_total_set_count(100)
				.with_descriptor(flags::DescriptorKind::UNIFORM_BUFFER, 100)
				.with_descriptor(flags::DescriptorKind::COMBINED_IMAGE_SAMPLER, 100)
				.build(logical)?,
		));

		Ok(RenderChain {
			physical: sync::Arc::downgrade(physical),
			logical: sync::Arc::downgrade(logical),
			allocator: sync::Arc::downgrade(allocator),
			graphics_queue: sync::Arc::new(graphics_queue),
			surface: sync::Arc::downgrade(surface),
			frame_count,
			camera: Camera::default(),

			persistent_descriptor_pool,

			transient_command_pool,
			frame_command_pool: None,
			command_buffers: Vec::new(),
			render_pass_instruction,
			render_pass_info: renderpass::Info::default().with_name("RenderPass"),
			is_dirty: true,
			resolution: [resolution.width as f32, resolution.height as f32].into(),

			render_pass: None,
			swapchain_info,
			swapchain: None,
			frame_images: Vec::new(),
			frame_image_views: Vec::new(),
			frame_buffers: Vec::new(),

			frame_command_buffer_requires_recording: vec![true; frame_count],
			img_available_semaphores: Vec::new(),
			render_finished_semaphores: Vec::new(),
			in_flight_fences: Vec::new(),
			images_in_flight: Vec::new(),
			current_frame: 0,

			pending_render_chain_elements: MultiMap::new(),
			initialized_render_chain_elements: MultiMap::new(),
		})
	}

	pub fn with_camera(mut self, camera: Camera) -> Self {
		self.set_camera(camera);
		self
	}

	pub fn set_camera(&mut self, camera: Camera) {
		self.camera = camera;
	}

	pub fn camera(&self) -> &Camera {
		&self.camera
	}

	/// Returns a pointer to the physical rendering device / GPU.
	pub fn physical(&self) -> sync::Arc<physical::Device> {
		self.physical.upgrade().unwrap()
	}

	/// Returns a pointer to the logical device for the GPU.
	pub fn logical(&self) -> sync::Arc<logical::Device> {
		self.logical.upgrade().unwrap()
	}

	/// Returns a pointer to the graphics object allocator (for creating buffers and images).
	pub fn allocator(&self) -> sync::Arc<graphics::alloc::Allocator> {
		self.allocator.upgrade().unwrap()
	}

	/// Returns a pointer to the command pool that should be used for one time submit / transient commands.
	/// This command pool is not dropped until the render chain is dropped.
	pub fn transient_command_pool(&self) -> &sync::Arc<command::Pool> {
		&self.transient_command_pool
	}

	/// Returns a pointer to the command pool that should be used for secondary command buffers
	/// (and is used for primary command buffers within the render chain).
	/// This command pool is dropped when destroying the render chain,
	/// and created when the render chain is constructed (often the same function call).
	pub fn frame_command_pool(&self) -> &command::Pool {
		self.frame_command_pool.as_ref().unwrap()
	}

	/// Returns a pointer to the logical queue for submitted graphics commands.
	pub fn graphics_queue(&self) -> &sync::Arc<logical::Queue> {
		&self.graphics_queue
	}

	/// Returns a mutex-pointer to the descriptor pool used for allocating all descriptor sets.
	/// This command pool is not dropped until the render chain is dropped.
	pub fn persistent_descriptor_pool(&self) -> &Arc<RwLock<graphics::descriptor::pool::Pool>> {
		&self.persistent_descriptor_pool
	}

	pub fn set_render_pass_info(&mut self, info: renderpass::Info) {
		self.render_pass_info = info;
		self.is_dirty = true;
	}

	/// Returns a reference to the render pass used to organize the render order.
	pub fn render_pass(&self) -> &renderpass::Pass {
		self.render_pass.as_ref().unwrap()
	}

	/// Adds a clear value to the render pass control so that a given frame is cleared when rendering begins.
	pub fn add_clear_value(&mut self, clear: renderpass::ClearValue) {
		self.render_pass_instruction.add_clear_value(clear);
	}

	/// Returns the number of frames that are being used/rendered.
	pub fn frame_count(&self) -> usize {
		self.frame_count
	}

	/// Returns the number of frames/images that can be in flight
	/// (being recorded to, being processed by the GPU commands, or being currently presented) at any given time.
	fn max_frames_in_flight(frame_count: usize) -> usize {
		std::cmp::max(frame_count - 1, 1)
	}

	/// Adds a rendering element to the chain for recording render commands.
	/// Elements will be initialized the next time `render_frame` is called.
	pub fn add_render_chain_element<T>(
		&mut self,
		subpass_id: Option<String>,
		element: &Arc<RwLock<T>>,
	) -> Result<(), AnyError>
	where
		T: 'static + graphics::RenderChainElement,
	{
		log::info!(
			target: graphics::LOG,
			"Adding render chain element {}",
			element.read().unwrap().name()
		);
		let arc: Arc<RwLock<dyn graphics::RenderChainElement>> = element.clone();
		self.pending_render_chain_elements
			.insert(subpass_id, Arc::downgrade(&arc));
		Ok(())
	}

	/// Creates the pipelines, command buffers, etc for the render chain, with a provided resolution.
	/// Any pre-existing pipelines and other objects will be dropped
	/// (and [`destroy_render_chain`](RenderChainElement::destroy_render_chain) will be called on any initialized elements).
	/// Initialized elements will get [`on_render_chain_constructed`](RenderChainElement::on_render_chain_constructed) called.
	#[profiling::function]
	fn construct_render_chain(&mut self, extent: structs::Extent2D) -> Result<(), AnyError> {
		log::info!(
			target: graphics::LOG,
			"{}Constructing render chain with resolution <{},{}>",
			self.render_pass.as_ref().map(|_| "re").unwrap_or(""),
			extent.width,
			extent.height
		);
		self.images_in_flight.clear();
		self.in_flight_fences.clear();
		self.render_finished_semaphores.clear();
		self.img_available_semaphores.clear();

		self.frame_buffers.clear();
		self.frame_image_views.clear();
		self.frame_images.clear();
		self.command_buffers.clear();

		self.initialized_render_chain_elements
			.retain(|_, element| element.strong_count() > 0);
		for (_, elements) in self.initialized_render_chain_elements.iter_all() {
			for element in elements.iter() {
				let arc = element.upgrade().unwrap();
				let mut locked = arc.write().unwrap();
				locked.destroy_render_chain(self)?;
			}
		}

		self.is_dirty = false;
		let physical = self.physical.upgrade().unwrap();
		let logical = self.logical.upgrade().unwrap();
		let surface = self.surface.upgrade().unwrap();

		self.swapchain_info.fill_from_physical(&physical);
		self.render_pass_instruction.set_extent(extent);

		self.frame_command_pool = Some(
			command::Pool::builder()
				.with_name("CommandPool.Frames")
				.with_queue_family_index(self.graphics_queue.index())
				.with_flag(flags::CommandPoolCreate::RESET_COMMAND_BUFFER)
				.build(&logical)?,
		);
		self.command_buffers = self
			.frame_command_pool()
			.allocate_buffers(self.frame_count, flags::CommandBufferLevel::PRIMARY)?;

		self.render_pass = Some(self.render_pass_info.clone().build(&logical)?);
		self.swapchain = Some(self.swapchain_info.clone().build(
			&logical,
			&surface,
			self.swapchain.as_ref(),
		)?);

		self.frame_images = self
			.swapchain()
			.get_images()?
			.into_iter()
			.map(|image| sync::Arc::new(image))
			.collect();
		for (i, image) in self.frame_images.iter().enumerate() {
			self.frame_image_views.push(
				image_view::View::builder()
					.with_name(format!("{}.Image.View", self.swapchain().frame_name(i)))
					.for_image(image.clone())
					.with_view_type(flags::ImageViewType::TYPE_2D)
					.with_range(
						structs::subresource::Range::default()
							.with_aspect(flags::ImageAspect::COLOR),
					)
					.build(&logical)?,
			);
		}
		for (i, image_view) in self.frame_image_views.iter().enumerate() {
			self.frame_buffers.push(
				command::framebuffer::Framebuffer::builder()
					.with_name(format!("{}.Framebuffer", self.swapchain().frame_name(i)))
					.set_extent(extent)
					.build(&image_view, &self.render_pass(), &logical)?,
			);
		}

		let max_frames_in_flight = RenderChain::max_frames_in_flight(self.frame_count);
		self.img_available_semaphores =
			RenderChain::create_semaphores(&logical, max_frames_in_flight)?;
		self.render_finished_semaphores =
			RenderChain::create_semaphores(&logical, max_frames_in_flight)?;
		self.in_flight_fences = RenderChain::create_fences(&logical, max_frames_in_flight)?;
		self.images_in_flight = self
			.frame_image_views
			.iter()
			.map(|_| None)
			.collect::<Vec<_>>();

		self.mark_commands_dirty();

		for (subpass_id, elements) in self.initialized_render_chain_elements.iter_all() {
			for element in elements.iter() {
				let arc = element.upgrade().unwrap();
				let mut locked = arc.write().unwrap();
				log::info!(
					target: graphics::LOG,
					"Constructing render chain for {}",
					locked.name()
				);
				locked.on_render_chain_constructed(self, &self.resolution, subpass_id)?;
			}
		}

		Ok(())
	}

	/// Marks all frames dirty, which results in the frames being re-recorded the next time they are rendered.
	pub fn mark_commands_dirty(&mut self) {
		self.frame_command_buffer_requires_recording = vec![true; self.frame_count];
	}

	fn swapchain(&self) -> &Swapchain {
		self.swapchain.as_ref().unwrap()
	}

	fn create_semaphores(
		logical: &sync::Arc<logical::Device>,
		amount: usize,
	) -> utility::Result<Vec<command::Semaphore>> {
		let mut vec: Vec<command::Semaphore> = Vec::new();
		for _ in 0..amount {
			vec.push((command::Semaphore::new(logical))?);
		}
		Ok(vec)
	}

	fn create_fences(
		logical: &sync::Arc<logical::Device>,
		amount: usize,
	) -> utility::Result<Vec<command::Fence>> {
		let mut vec: Vec<command::Fence> = Vec::new();
		for _ in 0..amount {
			vec.push((command::Fence::new(logical, flags::FenceState::SIGNALED))?);
		}
		Ok(vec)
	}

	/// Records commands for one frame to a command buffer.
	#[profiling::function]
	fn record_commands(&mut self, buffer_index: usize) -> Result<(), AnyError> {
		let use_secondary_buffers = false;
		let cmd = &mut self.command_buffers[buffer_index];

		cmd.begin(None, None)?;
		cmd.start_render_pass(
			&self.frame_buffers[buffer_index],
			self.render_pass.as_ref().unwrap(),
			self.render_pass_instruction.clone(),
			use_secondary_buffers,
		);

		let record_elements =
			|buffer: &mut command::Buffer, elements: &Vec<ChainElement>| -> Result<(), AnyError> {
				for element in elements.iter() {
					let arc = element.upgrade().unwrap();
					let locked = arc.read().unwrap();
					locked.record_to_buffer(buffer, buffer_index)?;
				}
				Ok(())
			};

		if let Some(prepass_elements) = self.initialized_render_chain_elements.get_vec(&None) {
			record_elements(cmd, prepass_elements)?;
		}

		let subpass_ids = self.render_pass_info.subpass_order();
		for idx in 0..subpass_ids.len() {
			let subpass_id = Some(subpass_ids[idx].clone());
			if let Some(elements) = self.initialized_render_chain_elements.get_vec(&subpass_id) {
				record_elements(cmd, elements)?;
			}
			if idx + 1 < subpass_ids.len() {
				cmd.next_subpass(use_secondary_buffers);
			}
		}

		cmd.stop_render_pass();
		cmd.end()?;

		Ok(())
	}

	/// Renders the next frame, thereby performing mutations like:
	/// initializing rendering elements,
	/// acquiring the next frame image,
	/// recording commands if necessary,
	/// submitting the command buffer to the GPU,
	/// and presenting the image to the window surface.
	///
	/// If the swapchain is out of date, then the render chain will
	/// destroy the display objects and reconstruct them
	/// (thereby causing all frames to be marked as dirty).
	#[profiling::function]
	pub fn render_frame(&mut self) -> Result<(), AnyError> {
		let logical = self.logical.upgrade().unwrap();

		let mut required_semaphores = Vec::new();
		let mut has_constructed_new_elements = false;
		let uninitialized_elements = self.pending_render_chain_elements.clone();
		self.pending_render_chain_elements.clear();
		for (subpass_id, elements) in uninitialized_elements.iter_all() {
			for element in elements.iter() {
				let rc = element.upgrade().unwrap();
				let mut locked = rc.write().unwrap();
				// initialize the item
				{
					log::info!(
						target: graphics::LOG,
						"Initializing {} for subpass {:?}",
						locked.name(),
						subpass_id
					);
					let mut found_semaphores = locked.initialize_with(self)?;
					required_semaphores.append(&mut found_semaphores);
					self.initialized_render_chain_elements
						.insert(subpass_id.clone(), element.clone());
				}
				// construct the chain if the chain already exists
				if !self.is_dirty {
					log::info!(
						target: graphics::LOG,
						"Constructing render chain for {}",
						locked.name()
					);
					locked.on_render_chain_constructed(self, &self.resolution, subpass_id)?;
					has_constructed_new_elements = true;
				}
			}
		}

		let pre_retain_element_count = self.initialized_render_chain_elements.len();
		self.initialized_render_chain_elements
			.retain(|_, element| element.strong_count() > 0);
		if has_constructed_new_elements
			|| pre_retain_element_count > self.initialized_render_chain_elements.len()
		{
			self.mark_commands_dirty();
		}

		for (_, elements) in self.initialized_render_chain_elements.iter_all() {
			for element in elements.iter() {
				let rc = element.upgrade().unwrap();
				let mut locked = rc.write().unwrap();
				locked.preframe_update(self)?;
			}
		}

		if self.is_dirty {
			logical.wait_until_idle()?;
			let extent = self
				.physical
				.upgrade()
				.unwrap()
				.query_surface_support()
				.image_extent();
			self.resolution = [extent.width as f32, extent.height as f32].into();
			if extent.width > 0 && extent.height > 0 {
				self.construct_render_chain(extent)?;
			}
		}

		// Wait for the previous frame/image to no longer be displayed
		logical.wait_for(&self.in_flight_fences[self.current_frame], u64::MAX)?;

		// Get the index of the next image to display
		let acquisition_result = self.swapchain().acquire_next_image(
			u64::MAX,
			Some(&self.img_available_semaphores[self.current_frame]),
			None,
		);
		let (next_image_idx, is_suboptimal) = match acquisition_result {
			Ok(data) => data,
			Err(e) => match e {
				crate::graphics::utility::Error::RequiresRenderChainUpdate() => {
					self.is_dirty = true;
					return Ok(());
				}
				_ => return Err(utility::Error::Graphics(e))?,
			},
		};
		let next_image_idx = next_image_idx as usize;
		if is_suboptimal {
			self.is_dirty = true;
			return Ok(());
		}

		// Ensure that the image for the next index is not being written to or displayed
		{
			let fence_index_for_img_in_flight = &self.images_in_flight[next_image_idx];
			if fence_index_for_img_in_flight.is_some() {
				logical.wait_for(
					&self.in_flight_fences[fence_index_for_img_in_flight.unwrap()],
					u64::MAX,
				)?;
			}
		}

		// Update any uniforms on pre-submit
		{
			for (_, elements) in self.initialized_render_chain_elements.iter_all() {
				for element in elements.iter() {
					let arc = element.upgrade().unwrap();
					let mut locked = arc.write().unwrap();
					if locked.prerecord_update(
						&self,
						&self.command_buffers[next_image_idx],
						next_image_idx,
						&self.resolution,
					)? {
						self.frame_command_buffer_requires_recording[next_image_idx] = true;
					}
				}
			}
		}

		if self.frame_command_buffer_requires_recording[next_image_idx] {
			self.record_commands(next_image_idx)?;
			self.frame_command_buffer_requires_recording[next_image_idx] = false;
		}

		// Denote that the image that is in-flight is the fence for the this frame
		self.images_in_flight[next_image_idx] = Some(self.current_frame);

		// Mark the image as not having been signaled (it is now being used)
		logical.reset_fences(&[&self.in_flight_fences[self.current_frame]])?;

		for (_, elements) in self.initialized_render_chain_elements.iter_all() {
			for element in elements.iter() {
				let rc = element.upgrade().unwrap();
				let mut locked = rc.write().unwrap();
				let mut found_semaphores = locked.take_gpu_signals();
				required_semaphores.append(&mut found_semaphores);
			}
		}

		self.graphics_queue.submit(
			vec![command::SubmitInfo::default()
				// tell the gpu to wait until the image is available
				.wait_for(
					&self.img_available_semaphores[self.current_frame],
					flags::PipelineStage::ColorAttachmentOutput,
				)
				.wait_for_semaphores(&required_semaphores)
				// denote which command buffer is being executed
				.add_buffer(&self.command_buffers[next_image_idx])
				// tell the gpu to signal a semaphore when the image is available again
				.signal_when_complete(&self.render_finished_semaphores[self.current_frame])],
			Some(&self.in_flight_fences[self.current_frame]),
		)?;

		let present_result = self.graphics_queue.present(
			command::PresentInfo::default()
				.wait_for(&self.render_finished_semaphores[self.current_frame])
				.add_swapchain(self.swapchain())
				.add_image_index(next_image_idx as u32),
		);
		match present_result {
			Ok(is_suboptimal) => {
				if is_suboptimal {
					self.is_dirty = true;
					return Ok(());
				}
			}
			Err(e) => match e {
				crate::graphics::utility::Error::RequiresRenderChainUpdate() => {
					self.is_dirty = true;
					return Ok(());
				}
				_ => return Err(utility::Error::Graphics(e))?,
			},
		}

		self.current_frame =
			(self.current_frame + 1) % RenderChain::max_frames_in_flight(self.frame_count);
		profiling::finish_frame!();
		Ok(())
	}
}

use crate::{
	graphics::{
		self, command,
		device::{logical, physical, swapchain},
		flags, image, image_view, renderpass, structs, Surface,
	},
	math::Vector,
	utility::{self, AnyError},
};
use std::sync::{self, Arc, RwLock, Weak};

/// An object which contains data that needs to be updated when the render-chain is reconstructed
/// (i.e. something which contains a pipeline, and is therefore reliant on the resolution of the window).
pub trait RenderChainElement: Send + Sync {
	/// Initializes the renderer before the first frame.
	/// Returns any semaphores which must complete before the first frame is submitted.
	fn initialize_with(
		&mut self,
		render_chain: &mut RenderChain,
	) -> utility::Result<Vec<Arc<command::Semaphore>>>;

	/// Creates any objects, like pipelines, which need to be created for a given swapchain (i.e. on window size change).
	/// Returns any semaphores which must complete before the next frame is submitted.
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()>;

	/// Returns a list of gpu semaphores that need to be completed/signaled before the next frame can be submitted.
	fn take_gpu_signals(&mut self) -> Vec<Arc<command::Semaphore>> {
		Vec::new()
	}

	/// Destroys any objects which are created during `on_render_chain_constructed`.
	fn destroy_render_chain(&mut self, render_chain: &RenderChain) -> utility::Result<()>;
}

/// An object which records commands to one or more command buffers,
/// notably when the render-chain is reconstructed.
pub trait CommandRecorder: Send + Sync {
	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> utility::Result<()>;
	fn update_pre_submit(
		&mut self,
		frame: usize,
		resolution: &Vector<u32, 2>,
	) -> utility::Result<()>;
}

type ChainElement = Weak<RwLock<dyn graphics::RenderChainElement>>;

pub struct RenderChain {
	command_recorders: Vec<Weak<RwLock<dyn graphics::CommandRecorder>>>,
	initialized_render_chain_elements: Vec<ChainElement>,
	pending_render_chain_elements: Vec<ChainElement>,

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
	swapchain: Option<swapchain::Swapchain>,
	render_pass: Option<renderpass::Pass>,

	swapchain_info: swapchain::Info,
	transient_command_pool: sync::Arc<command::Pool>,

	resolution: Vector<u32, 2>,
	is_dirty: bool,
	render_pass_info: renderpass::Info,
	render_pass_instruction: renderpass::RecordInstruction,

	frame_count: usize,

	task_spawner: sync::Arc<crate::task::Spawner>,
	persistent_descriptor_pool: sync::Arc<graphics::descriptor::pool::Pool>,
	surface: sync::Weak<Surface>,
	graphics_queue: sync::Arc<logical::Queue>,
	allocator: sync::Weak<graphics::alloc::Allocator>,
	logical: sync::Weak<logical::Device>,
	physical: sync::Weak<physical::Device>,
}

impl RenderChain {
	pub fn new(
		physical: &sync::Arc<physical::Device>,
		logical: &sync::Arc<logical::Device>,
		allocator: &sync::Arc<graphics::alloc::Allocator>,
		graphics_queue: logical::Queue,
		surface: &sync::Arc<Surface>,
		frame_count: usize,
		render_pass_info: renderpass::Info,
		task_spawner: sync::Arc<crate::task::Spawner>,
	) -> utility::Result<RenderChain> {
		let swapchain_info = swapchain::Info::default()
			.set_image_count(frame_count as u32)
			.set_image_format(flags::Format::B8G8R8A8_SRGB)
			.set_image_color_space(flags::ColorSpace::SRGB_NONLINEAR)
			.set_image_array_layer_count(1)
			.set_image_usage(flags::ImageUsageFlags::COLOR_ATTACHMENT)
			.set_image_sharing_mode(flags::SharingMode::EXCLUSIVE)
			.set_composite_alpha(flags::CompositeAlpha::OPAQUE)
			.set_is_clipped(true);
		let render_pass_instruction = renderpass::RecordInstruction::default();
		let resolution = physical.query_surface_support().image_extent();

		let transient_command_pool =
			sync::Arc::new(command::Pool::create(&logical, graphics_queue.index())?);

		let persistent_descriptor_pool = sync::Arc::new(
			graphics::descriptor::Pool::builder()
				.with_total_set_count(100)
				.with_descriptor(flags::DescriptorKind::UNIFORM_BUFFER, 100)
				.with_descriptor(flags::DescriptorKind::COMBINED_IMAGE_SAMPLER, 100)
				.build(logical)?,
		);

		Ok(RenderChain {
			physical: sync::Arc::downgrade(physical),
			logical: sync::Arc::downgrade(logical),
			allocator: sync::Arc::downgrade(allocator),
			graphics_queue: sync::Arc::new(graphics_queue),
			surface: sync::Arc::downgrade(surface),
			frame_count,
			task_spawner,

			persistent_descriptor_pool,

			transient_command_pool,
			frame_command_pool: None,
			command_buffers: Vec::new(),
			render_pass_instruction,
			render_pass_info,
			is_dirty: true,
			resolution: Vector::new([resolution.width, resolution.height]),

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

			command_recorders: Vec::new(),
			pending_render_chain_elements: Vec::new(),
			initialized_render_chain_elements: Vec::new(),
		})
	}

	pub fn task_spawner(&self) -> &Arc<crate::task::Spawner> {
		&self.task_spawner
	}

	pub fn physical(&self) -> sync::Arc<physical::Device> {
		self.physical.upgrade().unwrap()
	}

	pub fn logical(&self) -> sync::Arc<logical::Device> {
		self.logical.upgrade().unwrap()
	}

	pub fn allocator(&self) -> sync::Arc<graphics::alloc::Allocator> {
		self.allocator.upgrade().unwrap()
	}

	pub fn transient_command_pool(&self) -> &sync::Arc<command::Pool> {
		&self.transient_command_pool
	}

	pub fn graphics_queue(&self) -> &sync::Arc<logical::Queue> {
		&self.graphics_queue
	}

	pub fn persistent_descriptor_pool(&mut self) -> &mut graphics::descriptor::pool::Pool {
		sync::Arc::get_mut(&mut self.persistent_descriptor_pool).unwrap()
	}

	pub fn render_pass(&self) -> &renderpass::Pass {
		self.render_pass.as_ref().unwrap()
	}

	pub fn add_clear_value(&mut self, clear: renderpass::ClearValue) {
		self.render_pass_instruction.add_clear_value(clear);
	}

	pub fn frame_count(&self) -> usize {
		self.frame_count
	}

	fn max_frames_in_flight(frame_count: usize) -> usize {
		std::cmp::max(frame_count - 1, 1)
	}

	pub fn add_render_chain_element<T>(&mut self, element: &Arc<RwLock<T>>) -> Result<(), AnyError>
	where
		T: 'static + graphics::RenderChainElement,
	{
		let arc: Arc<RwLock<dyn graphics::RenderChainElement>> = element.clone();
		self.pending_render_chain_elements
			.push(Arc::downgrade(&arc));
		Ok(())
	}

	pub fn add_command_recorder<T>(&mut self, recorder: &Arc<RwLock<T>>) -> utility::Result<()>
	where
		T: 'static + graphics::CommandRecorder,
	{
		let arc: Arc<RwLock<dyn graphics::CommandRecorder>> = recorder.clone();
		self.command_recorders.push(Arc::downgrade(&arc));
		Ok(())
	}

	pub fn construct_render_chain(
		&mut self,
		resolution: structs::Extent2D,
	) -> Result<(), AnyError> {
		optick::event!();
		self.images_in_flight.clear();
		self.in_flight_fences.clear();
		self.render_finished_semaphores.clear();
		self.img_available_semaphores.clear();

		self.frame_buffers.clear();
		self.frame_image_views.clear();
		self.frame_images.clear();
		self.command_buffers.clear();

		self.initialized_render_chain_elements
			.retain(|element| element.strong_count() > 0);
		for element in self.initialized_render_chain_elements.iter() {
			let arc = element.upgrade().unwrap();
			let mut locked = arc.write().unwrap();
			locked.destroy_render_chain(self)?;
		}

		self.is_dirty = false;
		let physical = self.physical.upgrade().unwrap();
		let logical = self.logical.upgrade().unwrap();
		let surface = self.surface.upgrade().unwrap();

		self.swapchain_info.fill_from_physical(&physical);
		self.render_pass_instruction.set_extent(resolution);

		self.frame_command_pool = Some(command::Pool::create(
			&logical,
			self.graphics_queue.index(),
		)?);
		self.command_buffers = self
			.frame_command_pool
			.as_ref()
			.unwrap()
			.allocate_buffers(self.frame_count, flags::CommandBufferLevel::PRIMARY)?;

		self.render_pass = Some(self.render_pass_info.create_object(&logical)?);
		self.swapchain = Some(RenderChain::create_swapchain(
			&self.swapchain_info,
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
		self.frame_image_views = RenderChain::create_image_views(&logical, &self.frame_images)?;
		self.frame_buffers = RenderChain::create_frame_buffers(
			&self.frame_image_views,
			resolution,
			self.render_pass(),
			&logical,
		)?;

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

		for element in self.initialized_render_chain_elements.iter() {
			let arc = element.upgrade().unwrap();
			let mut locked = arc.write().unwrap();
			locked.on_render_chain_constructed(self, resolution)?;
		}

		Ok(())
	}

	pub fn mark_commands_dirty(&mut self) {
		self.frame_command_buffer_requires_recording = vec![true; self.frame_count];
	}

	fn create_swapchain(
		info: &swapchain::Info,
		logical: &sync::Arc<logical::Device>,
		surface: &Surface,
		old_swapchain: Option<&swapchain::Swapchain>,
	) -> utility::Result<swapchain::Swapchain> {
		Ok(info.create_object(logical, surface, old_swapchain)?)
	}

	fn swapchain(&self) -> &swapchain::Swapchain {
		self.swapchain.as_ref().unwrap()
	}

	fn create_image_views(
		logical: &sync::Arc<logical::Device>,
		frame_images: &Vec<sync::Arc<image::Image>>,
	) -> utility::Result<Vec<image_view::View>> {
		let mut views: Vec<image_view::View> = Vec::new();
		for image in frame_images.iter() {
			views.push(
				image_view::View::builder()
					.for_image(image.clone())
					.with_view_type(flags::ImageViewType::TYPE_2D)
					.with_format(flags::Format::B8G8R8A8_SRGB)
					.with_range(
						structs::subresource::Range::default()
							.with_aspect(flags::ImageAspect::COLOR),
					)
					.build(logical)?,
			);
		}
		Ok(views)
	}

	fn create_frame_buffers(
		views: &Vec<image_view::View>,
		extent: structs::Extent2D,
		render_pass: &renderpass::Pass,
		logical: &sync::Arc<logical::Device>,
	) -> utility::Result<Vec<command::framebuffer::Framebuffer>> {
		let mut frame_buffers: Vec<command::framebuffer::Framebuffer> = Vec::new();
		for image_view in views.iter() {
			frame_buffers.push(
				command::framebuffer::Info::default()
					.set_extent(extent)
					.create_object(&image_view, &render_pass, logical)?,
			);
		}
		Ok(frame_buffers)
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

	fn record_commands(&mut self, buffer_index: usize) -> Result<(), AnyError> {
		optick::event!();
		self.command_buffers[buffer_index].begin(None)?;
		self.command_buffers[buffer_index].start_render_pass(
			&self.frame_buffers[buffer_index],
			self.render_pass(),
			self.render_pass_instruction.clone(),
		);

		self.command_recorders
			.retain(|recorder| recorder.strong_count() > 0);
		for recorder in self.command_recorders.iter() {
			let arc = recorder.upgrade().unwrap();
			let locked = arc.read().unwrap();
			locked.record_to_buffer(&mut self.command_buffers[buffer_index], buffer_index)?;
		}

		self.command_buffers[buffer_index].stop_render_pass();
		self.command_buffers[buffer_index].end()?;

		Ok(())
	}

	pub fn render_frame(&mut self) -> Result<(), AnyError> {
		optick::next_frame();
		let logical = self.logical.upgrade().unwrap();

		let mut required_semaphores = Vec::new();
		let mut has_constructed_new_elements = false;
		let uninitialized_elements = self.pending_render_chain_elements.clone();
		self.pending_render_chain_elements.clear();
		for element in uninitialized_elements.iter() {
			let rc = element.upgrade().unwrap();
			let mut locked = rc.write().unwrap();
			// initialize the item
			{
				let mut found_semaphores = locked.initialize_with(self)?;
				required_semaphores.append(&mut found_semaphores);
				self.initialized_render_chain_elements.push(element.clone());
			}
			// construct the chain if the chain already exists
			if !self.is_dirty {
				locked.on_render_chain_constructed(
					self,
					structs::Extent2D {
						width: self.resolution.x(),
						height: self.resolution.y(),
					},
				)?;
				has_constructed_new_elements = true;
			}
		}

		if has_constructed_new_elements {
			self.mark_commands_dirty();
		}

		for element in self.initialized_render_chain_elements.iter() {
			let rc = element.upgrade().unwrap();
			let mut locked = rc.write().unwrap();
			let mut found_semaphores = locked.take_gpu_signals();
			required_semaphores.append(&mut found_semaphores);
		}

		if self.is_dirty {
			logical.wait_until_idle()?;
			let resolution = self
				.physical
				.upgrade()
				.unwrap()
				.query_surface_support()
				.image_extent();
			self.resolution = Vector::new([resolution.width, resolution.height]);
			if resolution.width > 0 && resolution.height > 0 {
				self.construct_render_chain(resolution)?;
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
				temportal_graphics::utility::Error::RequiresRenderChainUpdate() => {
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

		if self.frame_command_buffer_requires_recording[next_image_idx] {
			self.record_commands(next_image_idx)?;
			self.frame_command_buffer_requires_recording[next_image_idx] = false;
		}

		// Denote that the image that is in-flight is the fence for the this frame
		self.images_in_flight[next_image_idx] = Some(self.current_frame);

		// Mark the image as not having been signaled (it is now being used)
		logical.reset_fences(&[&self.in_flight_fences[self.current_frame]])?;

		// Update any uniforms on pre-submit
		{
			self.command_recorders
				.retain(|recorder| recorder.strong_count() > 0);
			for recorder in self.command_recorders.iter() {
				let arc = recorder.upgrade().unwrap();
				let mut locked = arc.write().unwrap();
				locked.update_pre_submit(next_image_idx, &self.resolution)?;
			}
		}

		self.graphics_queue.submit(
			vec![command::SubmitInfo::default()
				// tell the gpu to wait until the image is available
				.wait_for(
					&self.img_available_semaphores[self.current_frame],
					flags::PipelineStage::COLOR_ATTACHMENT_OUTPUT,
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
				temportal_graphics::utility::Error::RequiresRenderChainUpdate() => {
					self.is_dirty = true;
					return Ok(());
				}
				_ => return Err(utility::Error::Graphics(e))?,
			},
		}

		self.current_frame =
			(self.current_frame + 1) % RenderChain::max_frames_in_flight(self.frame_count);
		Ok(())
	}
}

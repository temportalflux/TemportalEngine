use crate::{
	graphics::{
		self, command,
		device::{logical, physical, swapchain},
		flags, image, image_view, renderpass, structs, Surface,
	},
	utility,
};
use std::{
	cell::RefCell,
	rc::{Rc, Weak},
};

/// An object which contains data that needs to be updated when the render-chain is reconstructed
/// (i.e. something which contains a pipeline, and is therefore reliant on the resolution of the window).
pub trait RenderChainElement {
	fn initialize_with(&mut self, render_chain: &RenderChain) -> utility::Result<()>;
	fn destroy_render_chain(&mut self, render_chain: &RenderChain) -> utility::Result<()>;
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()>;
}

/// An object which records commands to one or more command buffers,
/// notably when the render-chain is reconstructed.
pub trait CommandRecorder {
	fn record_to_buffer(&self, buffer: &mut command::Buffer) -> utility::Result<()>;
}

pub struct RenderChain {
	command_recorders: Vec<Weak<RefCell<dyn graphics::CommandRecorder>>>,
	render_chain_elements: Vec<Weak<RefCell<dyn graphics::RenderChainElement>>>,

	current_frame: usize,
	images_in_flight: Vec<Option</*in_flight_fence index*/ usize>>,
	in_flight_fences: Vec<command::Fence>,
	render_finished_semaphores: Vec<command::Semaphore>,
	img_available_semaphores: Vec<command::Semaphore>,
	frame_command_buffer_requires_recording: Vec<bool>,

	frame_buffers: Vec<command::framebuffer::Framebuffer>,
	frame_image_views: Vec<image_view::View>,
	frame_images: Vec<image::Image>,
	swapchain: swapchain::Swapchain,
	swapchain_info: swapchain::Info,
	render_pass: renderpass::Pass,
	command_buffers: Vec<command::Buffer>,
	frame_command_pool: command::Pool,
	transient_command_pool: command::Pool,

	is_dirty: bool,
	render_pass_info: renderpass::Info,
	render_pass_instruction: renderpass::RecordInstruction,

	frame_count: usize,

	persistent_descriptor_pool: RefCell<graphics::descriptor::pool::Pool>,
	surface: Weak<Surface>,
	graphics_queue: logical::Queue,
	allocator: Weak<graphics::alloc::Allocator>,
	logical: Weak<logical::Device>,
	physical: Weak<physical::Device>,

	window_id: u32,
}

impl RenderChain {
	pub fn new(
		window_id: u32,
		physical: &Rc<physical::Device>,
		logical: &Rc<logical::Device>,
		allocator: &Rc<graphics::alloc::Allocator>,
		graphics_queue: logical::Queue,
		surface: &Rc<Surface>,
		frame_count: usize,
		render_pass_info: renderpass::Info,
	) -> utility::Result<RenderChain> {
		let mut swapchain_info = swapchain::Info::default()
			.set_image_count(frame_count as u32)
			.set_image_format(flags::Format::B8G8R8A8_SRGB)
			.set_image_color_space(flags::ColorSpace::SRGB_NONLINEAR)
			.set_image_array_layer_count(1)
			.set_image_usage(flags::ImageUsageFlags::COLOR_ATTACHMENT)
			.set_image_sharing_mode(flags::SharingMode::EXCLUSIVE)
			.set_composite_alpha(flags::CompositeAlpha::OPAQUE)
			.set_is_clipped(true);
		let mut render_pass_instruction = renderpass::RecordInstruction::default();

		let resolution = physical.query_surface_support().image_extent();
		swapchain_info.fill_from_physical(&physical);
		render_pass_instruction.set_extent(resolution);

		let frame_command_pool =
			utility::as_graphics_error(command::Pool::create(&logical, graphics_queue.index()))?;
		let transient_command_pool =
			utility::as_graphics_error(command::Pool::create(&logical, graphics_queue.index()))?;

		let command_buffers = utility::as_graphics_error(
			frame_command_pool.allocate_buffers(frame_count, flags::CommandBufferLevel::PRIMARY),
		)?;

		let swapchain = RenderChain::create_swapchain(&swapchain_info, logical, surface, None)?;
		let frame_images = utility::as_graphics_error(swapchain.get_images())?;
		let frame_image_views = RenderChain::create_image_views(logical, &frame_images)?;
		let render_pass = utility::as_graphics_error(render_pass_info.create_object(&logical))?;
		let frame_buffers = RenderChain::create_frame_buffers(
			&frame_image_views,
			resolution,
			&render_pass,
			logical,
		)?;

		let max_frames_in_flight = RenderChain::max_frames_in_flight(frame_count);
		let img_available_semaphores =
			RenderChain::create_semaphores(logical, max_frames_in_flight)?;
		let render_finished_semaphores =
			RenderChain::create_semaphores(logical, max_frames_in_flight)?;
		let in_flight_fences = RenderChain::create_fences(logical, max_frames_in_flight)?;
		let images_in_flight: Vec<Option<usize>> =
			frame_image_views.iter().map(|_| None).collect::<Vec<_>>();

		let frame_command_buffer_requires_recording = vec![true; frame_count];

		let persistent_descriptor_pool = RefCell::new(utility::as_graphics_error(
			graphics::descriptor::Pool::builder()
				.with_total_set_count(100)
				.with_descriptor(flags::DescriptorKind::COMBINED_IMAGE_SAMPLER, 100)
				.build(logical),
		)?);

		Ok(RenderChain {
			window_id,
			physical: Rc::downgrade(physical),
			logical: Rc::downgrade(logical),
			allocator: Rc::downgrade(allocator),
			graphics_queue,
			surface: Rc::downgrade(surface),
			frame_count,

			persistent_descriptor_pool,

			transient_command_pool,
			frame_command_pool,
			command_buffers,
			render_pass_instruction,
			render_pass_info,
			is_dirty: false,

			render_pass,
			swapchain_info,
			swapchain,
			frame_images,
			frame_image_views,
			frame_buffers,

			frame_command_buffer_requires_recording,
			img_available_semaphores,
			render_finished_semaphores,
			in_flight_fences,
			images_in_flight,
			current_frame: 0,

			command_recorders: Vec::new(),
			render_chain_elements: Vec::new(),
		})
	}

	pub fn physical(&self) -> Rc<physical::Device> {
		self.physical.upgrade().unwrap()
	}

	pub fn logical(&self) -> Rc<logical::Device> {
		self.logical.upgrade().unwrap()
	}

	pub fn allocator(&self) -> Rc<graphics::alloc::Allocator> {
		self.allocator.upgrade().unwrap()
	}

	pub fn transient_command_pool(&self) -> &command::Pool {
		&self.transient_command_pool
	}

	pub fn graphics_queue(&self) -> &logical::Queue {
		&self.graphics_queue
	}

	pub fn persistent_descriptor_pool(&self) -> &RefCell<graphics::descriptor::pool::Pool> {
		&self.persistent_descriptor_pool
	}

	pub fn render_pass(&self) -> &renderpass::Pass {
		&self.render_pass
	}

	pub fn add_clear_value(&mut self, clear: renderpass::ClearValue) {
		self.render_pass_instruction.add_clear_value(clear);
	}

	fn max_frames_in_flight(frame_count: usize) -> usize {
		std::cmp::max(frame_count - 1, 1)
	}

	pub fn add_render_chain_element(
		&mut self,
		element: Rc<RefCell<dyn graphics::RenderChainElement>>,
	) -> utility::Result<()> {
		{
			let mut element_mut = element.borrow_mut();
			element_mut.initialize_with(&self)?;
			element_mut.on_render_chain_constructed(&self, *self.swapchain_info.image_extent())?;
		}
		self.render_chain_elements.push(Rc::downgrade(&element));
		Ok(())
	}

	pub fn add_command_recorder(
		&mut self,
		recorder: Rc<RefCell<dyn graphics::CommandRecorder>>,
	) -> utility::Result<()> {
		self.command_recorders.push(Rc::downgrade(&recorder));
		Ok(())
	}

	pub fn construct_render_chain(&mut self, resolution: structs::Extent2D) -> utility::Result<()> {
		optick::event!();
		self.images_in_flight.clear();
		self.in_flight_fences.clear();
		self.render_finished_semaphores.clear();
		self.img_available_semaphores.clear();

		self.frame_buffers.clear();
		self.frame_image_views.clear();
		self.frame_images.clear();
		self.command_buffers.clear();

		self.render_chain_elements
			.retain(|element| element.strong_count() > 0);
		for element in self.render_chain_elements.iter() {
			element
				.upgrade()
				.unwrap()
				.borrow_mut()
				.destroy_render_chain(self)?;
		}

		self.is_dirty = false;
		let physical = self.physical.upgrade().unwrap();
		let logical = self.logical.upgrade().unwrap();
		let surface = self.surface.upgrade().unwrap();

		self.swapchain_info.fill_from_physical(&physical);
		self.render_pass_instruction.set_extent(resolution);

		self.frame_command_pool = utility::as_graphics_error(command::Pool::create(
			&logical,
			self.graphics_queue.index(),
		))?;
		self.command_buffers = utility::as_graphics_error(
			self.frame_command_pool
				.allocate_buffers(self.frame_count, flags::CommandBufferLevel::PRIMARY),
		)?;

		self.render_pass =
			utility::as_graphics_error(self.render_pass_info.create_object(&logical))?;
		self.swapchain = RenderChain::create_swapchain(
			&self.swapchain_info,
			&logical,
			&surface,
			Some(&self.swapchain),
		)?;
		self.frame_images = utility::as_graphics_error(self.swapchain.get_images())?;
		self.frame_image_views = RenderChain::create_image_views(&logical, &self.frame_images)?;
		self.frame_buffers = RenderChain::create_frame_buffers(
			&self.frame_image_views,
			resolution,
			&self.render_pass,
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

		for element in self.render_chain_elements.iter() {
			element
				.upgrade()
				.unwrap()
				.borrow_mut()
				.on_render_chain_constructed(self, resolution)?;
		}

		Ok(())
	}

	pub fn mark_commands_dirty(&mut self) {
		self.frame_command_buffer_requires_recording = vec![true; self.frame_count];
	}

	fn create_swapchain(
		info: &swapchain::Info,
		logical: &Rc<logical::Device>,
		surface: &Surface,
		old_swapchain: Option<&swapchain::Swapchain>,
	) -> utility::Result<swapchain::Swapchain> {
		utility::as_graphics_error(info.create_object(logical, surface, old_swapchain))
	}

	fn create_image_views(
		logical: &Rc<logical::Device>,
		frame_images: &Vec<image::Image>,
	) -> utility::Result<Vec<image_view::View>> {
		let mut views: Vec<image_view::View> = Vec::new();
		for image in frame_images.iter() {
			views.push(utility::as_graphics_error(
				image_view::View::builder()
					.set_view_type(flags::ImageViewType::TYPE_2D)
					.set_format(flags::Format::B8G8R8A8_SRGB)
					.set_subresource_range(
						structs::subresource::Range::default()
							.with_aspect(flags::ImageAspect::COLOR),
					)
					.create_object(logical, &image),
			)?);
		}
		Ok(views)
	}

	fn create_frame_buffers(
		views: &Vec<image_view::View>,
		extent: structs::Extent2D,
		render_pass: &renderpass::Pass,
		logical: &Rc<logical::Device>,
	) -> utility::Result<Vec<command::framebuffer::Framebuffer>> {
		let mut frame_buffers: Vec<command::framebuffer::Framebuffer> = Vec::new();
		for image_view in views.iter() {
			frame_buffers.push(utility::as_graphics_error(
				command::framebuffer::Info::default()
					.set_extent(extent)
					.create_object(&image_view, &render_pass, logical),
			)?);
		}
		Ok(frame_buffers)
	}

	fn create_semaphores(
		logical: &Rc<logical::Device>,
		amount: usize,
	) -> utility::Result<Vec<command::Semaphore>> {
		let mut vec: Vec<command::Semaphore> = Vec::new();
		for _ in 0..amount {
			vec.push(utility::as_graphics_error(command::Semaphore::new(
				logical,
			))?);
		}
		Ok(vec)
	}

	fn create_fences(
		logical: &Rc<logical::Device>,
		amount: usize,
	) -> utility::Result<Vec<command::Fence>> {
		let mut vec: Vec<command::Fence> = Vec::new();
		for _ in 0..amount {
			vec.push(utility::as_graphics_error(command::Fence::new(
				logical,
				flags::FenceState::SIGNALED,
			))?);
		}
		Ok(vec)
	}

	fn record_commands(&mut self, buffer_index: usize) -> utility::Result<()> {
		optick::event!();
		utility::as_graphics_error(self.command_buffers[buffer_index].begin(None))?;
		self.command_buffers[buffer_index].start_render_pass(
			&self.frame_buffers[buffer_index],
			&self.render_pass,
			self.render_pass_instruction.clone(),
		);

		self.command_recorders
			.retain(|recorder| recorder.strong_count() > 0);
		for recorder in self.command_recorders.iter() {
			recorder
				.upgrade()
				.unwrap()
				.borrow_mut()
				.record_to_buffer(&mut self.command_buffers[buffer_index])?;
		}

		self.command_buffers[buffer_index].stop_render_pass();
		utility::as_graphics_error(self.command_buffers[buffer_index].end())?;

		Ok(())
	}

	pub fn render_frame(&mut self) -> utility::Result<()> {
		optick::next_frame();
		let logical = self.logical.upgrade().unwrap();

		if self.is_dirty {
			utility::as_graphics_error(logical.wait_until_idle())?;
			let resolution = self
				.physical
				.upgrade()
				.unwrap()
				.query_surface_support()
				.image_extent();
			if resolution.width > 0 && resolution.height > 0 {
				self.construct_render_chain(resolution)?;
			}
		}

		// Wait for the previous frame/image to no longer be displayed
		utility::as_graphics_error(logical.wait_for(
			&self.in_flight_fences[self.current_frame],
			true,
			u64::MAX,
		))?;

		// Get the index of the next image to display
		let acquisition_result = self.swapchain.acquire_next_image(
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
				_ => return Err(utility::Error::Graphics(e)),
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
				utility::as_graphics_error(logical.wait_for(
					&self.in_flight_fences[fence_index_for_img_in_flight.unwrap()],
					true,
					u64::MAX,
				))?;
			}
		}

		if self.frame_command_buffer_requires_recording[next_image_idx] {
			self.record_commands(next_image_idx)?;
			self.frame_command_buffer_requires_recording[next_image_idx] = false;
		}

		// Denote that the image that is in-flight is the fence for the this frame
		self.images_in_flight[next_image_idx] = Some(self.current_frame);

		// Mark the image as not having been signaled (it is now being used)
		utility::as_graphics_error(
			logical.reset_fences(&[&self.in_flight_fences[self.current_frame]]),
		)?;

		utility::as_graphics_error(self.graphics_queue.submit(
			vec![command::SubmitInfo::default()
				// tell the gpu to wait until the image is available
				.wait_for(
					&self.img_available_semaphores[self.current_frame],
					flags::PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				)
				// denote which command buffer is being executed
				.add_buffer(&self.command_buffers[next_image_idx])
				// tell the gpu to signal a semaphore when the image is available again
				.signal_when_complete(&self.render_finished_semaphores[self.current_frame])],
			Some(&self.in_flight_fences[self.current_frame]),
		))?;

		let present_result = self.graphics_queue.present(
			command::PresentInfo::default()
				.wait_for(&self.render_finished_semaphores[self.current_frame])
				.add_swapchain(&self.swapchain)
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
				_ => return Err(utility::Error::Graphics(e)),
			},
		}

		self.current_frame =
			(self.current_frame + 1) % RenderChain::max_frames_in_flight(self.frame_count);
		Ok(())
	}
}

impl crate::display::EventListener for RenderChain {
	fn on_event(&mut self, event: &sdl2::event::Event) -> bool {
		match event {
			sdl2::event::Event::Window {
				window_id,
				win_event: sdl2::event::WindowEvent::Resized(w, h),
				..
			} if *window_id == self.window_id => {
				log::debug!("Resized window {} to {}x{}", self.window_id, w, h);
			}
			_ => {}
		}
		false
	}
}

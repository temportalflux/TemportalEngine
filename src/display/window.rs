use crate::{display, graphics, utility, Engine};
use sdl2;
use std::{
	cell::RefCell,
	rc::{Rc, Weak},
};
use temportal_graphics::{
	command,
	device::{logical, physical, swapchain},
	flags, image, instance, renderpass, structs, Surface,
};

pub struct Window {
	current_frame: usize,
	images_in_flight: Vec<Option</*in_flight_fence index*/ usize>>,
	in_flight_fences: Vec<command::Fence>,
	render_finished_semaphores: Vec<command::Semaphore>,
	img_available_semaphores: Vec<command::Semaphore>,
	frame_command_buffer_requires_recording: Vec<bool>,
	graphics_queue: Option<logical::Queue>,

	command_recorders: Vec<Weak<RefCell<dyn graphics::CommandRecorder>>>,
	render_chain_elements: Vec<Weak<RefCell<dyn graphics::RenderChainElement>>>,

	render_pass_instruction: renderpass::RecordInstruction,
	command_buffers: Vec<command::Buffer>,
	command_pool: Option<command::Pool>,

	frame_buffers: Vec<command::framebuffer::Framebuffer>,
	render_pass: Option<renderpass::Pass>,

	frame_image_views: Vec<image::View>,
	frame_images: Vec<image::Image>,
	frame_count: usize,
	swapchain: Option<swapchain::Swapchain>,
	logical_device: Option<Rc<logical::Device>>,
	graphics_queue_index: Option<usize>,
	physical_device: Option<physical::Device>,
	surface: Surface,

	// This is at the bottom to ensure that rust deallocates it last
	vulkan: Rc<instance::Instance>,
	internal: WinWrapper,
	engine: Rc<RefCell<Engine>>,
}

impl Window {
	pub fn new(
		engine: &Rc<RefCell<Engine>>,
		sdl_window: sdl2::video::Window,
	) -> utility::Result<Window> {
		let internal = WinWrapper {
			internal: sdl_window,
		};
		let eng = engine.borrow();
		let instance = utility::as_graphics_error(
			instance::Info::default()
				.set_app_info(eng.app_info.clone())
				.set_window(&internal)
				.set_use_validation(eng.vulkan_validation_enabled)
				.create_object(&eng.graphics_context),
		)?;
		let vulkan = std::rc::Rc::new(instance);
		let surface =
			utility::as_graphics_error(instance::Instance::create_surface(&vulkan, &internal))?;
		Ok(Window {
			engine: engine.clone(),
			internal,
			vulkan,
			surface,
			physical_device: None,
			logical_device: None,
			graphics_queue_index: None,
			swapchain: None,
			frame_count: 0,
			frame_images: Vec::new(),
			frame_image_views: Vec::new(),
			render_pass: None,
			frame_buffers: Vec::new(),
			frame_command_buffer_requires_recording: Vec::new(),
			command_pool: None,
			command_buffers: Vec::new(),
			render_pass_instruction: renderpass::RecordInstruction::default(),
			command_recorders: Vec::new(),
			render_chain_elements: Vec::new(),
			graphics_queue: None,
			img_available_semaphores: Vec::new(),
			render_finished_semaphores: Vec::new(),
			in_flight_fences: Vec::new(),
			images_in_flight: Vec::new(),
			current_frame: 0,
		})
	}

	fn id(&self) -> u32 {
		self.internal.internal.id()
	}

	pub fn add_render_chain_element(
		&mut self,
		element: Rc<RefCell<dyn graphics::RenderChainElement>>,
	) -> utility::Result<()> {
		element.borrow_mut().initialize_with(&self)?;
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
}

struct WinWrapper {
	internal: sdl2::video::Window,
}

unsafe impl raw_window_handle::HasRawWindowHandle for WinWrapper {
	fn raw_window_handle(&self) -> raw_window_handle::RawWindowHandle {
		self.internal.raw_window_handle()
	}
}

unsafe impl raw_window_handle::HasRawWindowHandle for Window {
	fn raw_window_handle(&self) -> raw_window_handle::RawWindowHandle {
		self.internal.raw_window_handle()
	}
}

impl display::EventListener for Window {
	fn on_event(&mut self, event: &sdl2::event::Event) -> bool {
		match event {
			sdl2::event::Event::Window {
				window_id,
				win_event: sdl2::event::WindowEvent::Resized(w, h),
				..
			} if *window_id == self.id() => {
				println!("Resized window {} to {}x{}", self.id(), w, h);
			}
			_ => {}
		}
		false
	}
}

impl Window {
	pub fn find_physical_device(
		&mut self,
		constraints: &mut Vec<physical::Constraint>,
	) -> utility::Result<()> {
		constraints.push(physical::Constraint::HasQueueFamily(
			flags::QueueFlags::GRAPHICS,
			/*requires_surface*/ true,
		));
		self.physical_device = match self
			.vulkan
			.find_physical_device(&constraints, &self.surface)
		{
			Ok(device) => Some(device),
			Err(failed_constraint) => match failed_constraint {
				None => panic!("Failed to find any rendering device (do you not have anyu GPUs?)"),
				Some(constraint) => panic!(
					"Failed to find physical device, failed on constraint {:?}",
					constraint
				),
			},
		};
		println!("Found physical device {}", self.physical());

		let permitted_frame_count = self.physical().image_count_range();
		self.frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start as usize),
			permitted_frame_count.end as usize,
		);

		Ok(())
	}

	pub fn surface(&self) -> &Surface {
		&self.surface
	}

	pub fn physical(&self) -> &physical::Device {
		&self.physical_device.as_ref().unwrap()
	}

	pub fn create_logical(&mut self) -> utility::Result<()> {
		self.graphics_queue_index = self
			.physical()
			.get_queue_index(flags::QueueFlags::GRAPHICS, true);
		let queue_idx = self.graphics_queue_index();
		self.logical_device = Some(std::rc::Rc::new(utility::as_graphics_error(
			logical::Info::default()
				.add_extension("VK_KHR_swapchain")
				.set_validation_enabled(self.engine.borrow().vulkan_validation_enabled)
				.add_queue(logical::DeviceQueue {
					queue_family_index: queue_idx,
					priorities: vec![1.0],
				})
				.create_object(&self.vulkan, &self.physical()),
		)?));
		self.graphics_queue = Some(logical::Device::get_queue(
			self.logical(),
			self.graphics_queue_index(),
		));

		assert!(self.frame_count > 0);

		self.command_pool = Some(utility::as_graphics_error(command::Pool::create(
			&self.logical(),
			self.graphics_queue_index(),
		))?);
		self.command_buffers = utility::as_graphics_error(
			self.command_pool
				.as_ref()
				.unwrap()
				.allocate_buffers(self.frame_count),
		)?;

		Ok(())
	}

	pub fn logical(&self) -> &Rc<logical::Device> {
		&self.logical_device.as_ref().unwrap()
	}

	pub fn graphics_queue_index(&self) -> usize {
		self.graphics_queue_index.unwrap()
	}

	// TODO: Reconstruct during `create_render_chain`
	pub fn create_render_pass(&mut self, info: renderpass::Info) -> utility::Result<()> {
		self.render_pass = Some(utility::as_graphics_error(
			info.create_object(&self.logical()),
		)?);
		Ok(())
	}

	pub fn render_pass(&self) -> &renderpass::Pass {
		&self.render_pass.as_ref().unwrap()
	}

	pub fn command_buffers(&self) -> &Vec<command::Buffer> {
		&self.command_buffers
	}

	pub fn create_render_chain(&mut self) -> utility::Result<()> {
		assert!(self.frame_count > 0);

		self.frame_image_views.clear();
		self.frame_images.clear();
		self.img_available_semaphores.clear();
		self.render_finished_semaphores.clear();
		self.in_flight_fences.clear();
		self.images_in_flight.clear();

		self.render_pass_instruction
			.set_extent(self.physical().image_extent());

		self.swapchain = Some(utility::as_graphics_error(
			swapchain::Info::default()
				.set_image_count(self.frame_count as u32)
				.set_image_format(flags::Format::B8G8R8A8_SRGB)
				.set_image_color_space(flags::ColorSpace::SRGB_NONLINEAR_KHR)
				.set_image_extent(self.physical().image_extent())
				.set_image_array_layer_count(1)
				.set_image_usage(flags::ImageUsageFlags::COLOR_ATTACHMENT)
				.set_image_sharing_mode(flags::SharingMode::EXCLUSIVE)
				.set_pre_transform(self.physical().current_transform())
				.set_composite_alpha(flags::CompositeAlpha::OPAQUE_KHR)
				.set_present_mode(self.physical().selected_present_mode)
				.set_is_clipped(true)
				.create_object(self.logical(), &self.surface(), self.swapchain.as_ref()),
		)?);

		self.frame_images =
			utility::as_graphics_error(self.swapchain.as_ref().unwrap().get_images())?;

		for image in self.frame_images.iter() {
			self.frame_image_views.push(utility::as_graphics_error(
				image::ViewInfo::new()
					.set_view_type(flags::ImageViewType::_2D)
					.set_format(flags::Format::B8G8R8A8_SRGB)
					.set_subresource_range(structs::ImageSubresourceRange {
						aspect_mask: flags::ImageAspect::COLOR,
						base_mip_level: 0,
						level_count: 1,
						base_array_layer: 0,
						layer_count: 1,
					})
					.create_object(self.logical(), &image),
			)?);
		}

		self.frame_buffers.clear();
		for image_view in self.frame_image_views.iter() {
			let buffer_result = command::framebuffer::Info::default()
				.set_extent(self.physical().image_extent())
				.create_object(&image_view, &self.render_pass(), &self.logical());
			let buffer = utility::as_graphics_error(buffer_result)?;
			self.frame_buffers.push(buffer);
		}

		self.img_available_semaphores = utility::as_graphics_error(
			logical::Device::create_semaphores(&self.logical(), self.max_frames_in_flight()),
		)?;
		self.render_finished_semaphores = utility::as_graphics_error(
			logical::Device::create_semaphores(&self.logical(), self.max_frames_in_flight()),
		)?;
		self.in_flight_fences = utility::as_graphics_error(logical::Device::create_fences(
			&self.logical(),
			self.max_frames_in_flight(),
			flags::FenceState::SIGNALED,
		))?;
		self.current_frame = 0;
		self.images_in_flight = self.frame_views().iter().map(|_| None).collect::<Vec<_>>();

		self.render_chain_elements
			.retain(|element| element.strong_count() > 0);
		for element in self.render_chain_elements.iter() {
			element
				.upgrade()
				.unwrap()
				.borrow_mut()
				.on_render_chain_constructed(self)?;
		}

		self.mark_commands_dirty();

		Ok(())
	}

	pub fn swapchain(&self) -> &swapchain::Swapchain {
		&self.swapchain.as_ref().unwrap()
	}

	pub fn frame_views(&self) -> &Vec<image::View> {
		&self.frame_image_views
	}

	fn max_frames_in_flight(&self) -> usize {
		std::cmp::max(self.frame_count - 1, 1)
	}

	pub fn frame_buffers(&self) -> &Vec<command::framebuffer::Framebuffer> {
		&self.frame_buffers
	}

	pub fn add_clear_value(&mut self, clear: renderpass::ClearValue) {
		self.render_pass_instruction.add_clear_value(clear);
	}

	pub fn record_instruction(&self) -> &renderpass::RecordInstruction {
		&self.render_pass_instruction
	}

	pub fn mark_commands_dirty(&mut self) {
		self.frame_command_buffer_requires_recording = vec![true; self.command_buffers.len()];
	}

	fn record_commands(&mut self, buffer_index: usize) -> utility::Result<()> {
		utility::as_graphics_error(self.command_buffers[buffer_index].begin())?;
		self.command_buffers[buffer_index].start_render_pass(
			&self.frame_buffers[buffer_index],
			&self.render_pass(),
			self.record_instruction().clone(),
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
		// Wait for the previous frame/image to no longer be displayed
		utility::as_graphics_error(self.logical().wait_for(
			&self.in_flight_fences[self.current_frame],
			true,
			u64::MAX,
		))?;
		// Get the index of the next image to display
		let next_image_idx = utility::as_graphics_error(self.swapchain().acquire_next_image(
			u64::MAX,
			Some(&self.img_available_semaphores[self.current_frame]),
			None,
		))?;

		// Ensure that the image for the next index is not being written to or displayed
		{
			let fence_index_for_img_in_flight = &self.images_in_flight[next_image_idx];
			if fence_index_for_img_in_flight.is_some() {
				utility::as_graphics_error(self.logical().wait_for(
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
			self.logical()
				.reset_fences(&[&self.in_flight_fences[self.current_frame]]),
		)?;

		utility::as_graphics_error(self.graphics_queue.as_ref().unwrap().submit(
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

		utility::as_graphics_error(
			self.graphics_queue.as_ref().unwrap().present(
				command::PresentInfo::default()
					.wait_for(&self.render_finished_semaphores[self.current_frame])
					.add_swapchain(&self.swapchain())
					.add_image_index(next_image_idx as u32),
			),
		)?;

		self.current_frame = (self.current_frame + 1) % self.max_frames_in_flight();
		Ok(())
	}
}

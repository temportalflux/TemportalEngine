use crate::{display, graphics, utility, Engine};
use sdl2;
use std::{cell::RefCell, rc::Rc};
use temportal_graphics::{
	device::{logical, physical},
	flags, instance, renderpass, Surface,
};

pub struct WindowBuilder {
	title: String,
	width: u32,
	height: u32,
	constraints: Vec<physical::Constraint>,
}

impl Default for WindowBuilder {
	fn default() -> WindowBuilder {
		WindowBuilder {
			title: String::default(),
			width: 0,
			height: 0,
			constraints: Vec::new(),
		}
	}
}

impl WindowBuilder {
	pub fn title(mut self, title: &str) -> Self {
		self.title = title.to_string();
		self
	}

	pub fn size(mut self, width: u32, height: u32) -> Self {
		self.width = width;
		self.height = height;
		self
	}

	pub fn constraints(mut self, constraints: Vec<physical::Constraint>) -> Self {
		self.constraints = constraints;
		self
	}

	pub fn build(self, display: &mut display::Manager) -> utility::Result<Rc<RefCell<Window>>> {
		let sdl_window = display.create_sdl_window(self.title.as_str(), self.width, self.height)?;
		Ok(Rc::new(RefCell::new(display::Window::new(
			&display.engine(),
			sdl_window,
			self.constraints,
		)?)))
	}
}

pub struct Window {
	logical_device: Rc<logical::Device>,
	graphics_queue_index: usize,
	physical_device: Rc<physical::Device>,
	surface: Rc<Surface>,

	// This is at the bottom to ensure that rust deallocates it last
	_vulkan: Rc<instance::Instance>,
	internal: WinWrapper,
}

impl Window {
	pub fn new(
		engine: &Rc<RefCell<Engine>>,
		sdl_window: sdl2::video::Window,
		constraints: Vec<physical::Constraint>,
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
		let surface = Rc::new(utility::as_graphics_error(
			instance::Instance::create_surface(&eng.graphics_context, &vulkan, &internal),
		)?);

		let physical_device = Rc::new(Window::find_physical_device(
			&vulkan,
			&surface,
			constraints,
		)?);
		log::info!(
			target: display::LOG,
			"Window \"{}\" found physical device {}",
			internal.internal.title(),
			physical_device
		);

		let graphics_queue_index = physical_device
			.get_queue_index(flags::QueueFlags::GRAPHICS, true)
			.unwrap();
		let logical_device = std::rc::Rc::new(utility::as_graphics_error(
			logical::Info::default()
				.add_extension("VK_KHR_swapchain")
				.set_validation_enabled(engine.borrow().vulkan_validation_enabled)
				.add_queue(logical::DeviceQueue {
					queue_family_index: graphics_queue_index,
					priorities: vec![1.0],
				})
				.create_object(&vulkan, &physical_device),
		)?);

		Ok(Window {
			internal,
			_vulkan: vulkan,
			surface,
			physical_device,
			logical_device,
			graphics_queue_index,
		})
	}

	fn id(&self) -> u32 {
		self.internal.internal.id()
	}

	fn find_physical_device(
		vulkan: &Rc<instance::Instance>,
		surface: &Rc<Surface>,
		constraints: Vec<physical::Constraint>,
	) -> utility::Result<physical::Device> {
		let mut constraints = constraints.clone();
		constraints.push(physical::Constraint::HasQueueFamily(
			flags::QueueFlags::GRAPHICS,
			/*requires_surface*/ true,
		));
		match instance::Instance::find_physical_device(&vulkan, &constraints, &surface) {
			Ok(device) => Ok(device),
			Err(failed_constraint) => Err(utility::Error::FailedToFindPhysicalDevice(
				failed_constraint,
			)),
		}
	}

	pub fn create_render_chain(
		&self,
		display: &mut display::Manager,
		render_pass_info: renderpass::Info,
	) -> utility::Result<Rc<RefCell<graphics::RenderChain>>> {
		let permitted_frame_count = self
			.physical_device
			.query_surface_support()
			.image_count_range();
		let frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start as usize),
			permitted_frame_count.end as usize,
		);

		let graphics_queue =
			logical::Device::get_queue(&self.logical_device, self.graphics_queue_index);

		let render_chain_raw = graphics::RenderChain::new(
			self.id(),
			&self.physical_device,
			&self.logical_device,
			graphics_queue,
			&self.surface,
			frame_count,
			render_pass_info,
		)?;
		let render_chain = Rc::new(RefCell::new(render_chain_raw));
		let render_chain_weak = Rc::downgrade(&render_chain);
		display.add_event_listener(render_chain_weak);

		Ok(render_chain)
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

use crate::{utility, Engine};
use sdl2;
use std::rc::Rc;
use temportal_graphics::{
	device::{logical, physical},
	flags, instance, Surface,
};

pub struct Window {
	logical_device: Option<Rc<logical::Device>>,
	graphics_queue_index: Option<usize>,
	physical_device: Option<physical::Device>,
	surface: Surface,

	// This is at the bottom to ensure that rust deallocates it last
	vulkan: Rc<instance::Instance>,
	internal: WinWrapper,
	engine: Rc<Engine>,
}

impl Window {
	pub fn new(engine: &Rc<Engine>, sdl_window: sdl2::video::Window) -> utility::Result<Window> {
		let internal = WinWrapper {
			internal: sdl_window,
		};
		let instance = utility::as_graphics_error(
			instance::Info::default()
				.set_app_info(engine.app_info.clone())
				.set_window(&internal)
				.set_use_validation(engine.vulkan_validation_enabled)
				.create_object(&engine.graphics_context),
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
		})
	}

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
		Ok(())
	}

	pub fn surface(&self) -> &Surface {
		&self.surface
	}

	pub fn physical(&self) -> &physical::Device {
		&self.physical_device.as_ref().unwrap()
	}

	pub fn create_logical(&mut self) {
		self.graphics_queue_index = self
			.physical()
			.get_queue_index(flags::QueueFlags::GRAPHICS, true);
		let queue_idx = self.graphics_queue_index();
		self.logical_device = Some(std::rc::Rc::new(
			logical::Info::default()
				.add_extension("VK_KHR_swapchain")
				.set_validation_enabled(self.engine.vulkan_validation_enabled)
				.add_queue(logical::DeviceQueue {
					queue_family_index: queue_idx,
					priorities: vec![1.0],
				})
				.create_object(&self.vulkan, &self.physical()),
		));
	}

	pub fn logical(&self) -> &Rc<logical::Device> {
		&self.logical_device.as_ref().unwrap()
	}

	pub fn graphics_queue_index(&self) -> usize {
		self.graphics_queue_index.unwrap()
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

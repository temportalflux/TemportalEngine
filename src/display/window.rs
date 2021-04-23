use crate::{display, graphics, utility, Engine};
use sdl2;
use std::{cell::RefCell, rc::Rc};
use temportal_graphics::{
	command,
	device::{logical, physical},
	flags, instance, renderpass, Surface,
};

pub struct Window {
	command_pool: Option<command::Pool>,
	logical_device: Option<Rc<logical::Device>>,
	graphics_queue_index: Option<usize>,
	physical_device: Option<Rc<physical::Device>>,
	surface: Rc<Surface>,

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
			surface: Rc::new(surface),
			physical_device: None,
			logical_device: None,
			graphics_queue_index: None,
			command_pool: None,
		})
	}

	fn id(&self) -> u32 {
		self.internal.internal.id()
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
				log::debug!("Resized window {} to {}x{}", self.id(), w, h);
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
			Ok(device) => Some(Rc::new(device)),
			Err(failed_constraint) => match failed_constraint {
				None => panic!("Failed to find any rendering device (do you not have anyu GPUs?)"),
				Some(constraint) => panic!(
					"Failed to find physical device, failed on constraint {:?}",
					constraint
				),
			},
		};
		log::info!(
			target: display::LOG,
			"Window {} found physical device {}",
			self.internal.internal.title(),
			self.physical()
		);

		Ok(())
	}

	pub fn surface(&self) -> &Surface {
		&self.surface
	}

	pub fn physical(&self) -> &Rc<physical::Device> {
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

		self.command_pool = Some(utility::as_graphics_error(command::Pool::create(
			&self.logical(),
			self.graphics_queue_index(),
		))?);

		Ok(())
	}

	pub fn logical(&self) -> &Rc<logical::Device> {
		&self.logical_device.as_ref().unwrap()
	}

	pub fn graphics_queue_index(&self) -> usize {
		self.graphics_queue_index.unwrap()
	}

	pub fn create_render_chain(
		&self,
		render_pass_info: renderpass::Info,
	) -> utility::Result<graphics::RenderChain> {
		let permitted_frame_count = self.physical().image_count_range();
		let frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start as usize),
			permitted_frame_count.end as usize,
		);

		let graphics_queue =
			logical::Device::get_queue(self.logical(), self.graphics_queue_index());

		graphics::RenderChain::new(
			self.physical(),
			self.logical(),
			graphics_queue,
			&self.surface,
			frame_count,
			render_pass_info,
			self.command_pool.as_ref().unwrap(),
		)
	}
}

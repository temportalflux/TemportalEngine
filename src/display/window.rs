use crate::{display, graphics, utility, Application};
use sdl2;
use std::sync;
use temportal_graphics::{
	device::{logical, physical},
	flags, instance, renderpass, AppInfo, Context, Surface,
};

fn is_vulkan_validation_enabled() -> bool {
	cfg!(debug_assertions)
}

pub struct WindowBuilder {
	app_info: AppInfo,
	title: String,
	width: u32,
	height: u32,
	constraints: Vec<physical::Constraint>,
	resizable: bool,
}

impl Default for WindowBuilder {
	fn default() -> WindowBuilder {
		WindowBuilder {
			app_info: AppInfo::default(),
			title: String::default(),
			width: 0,
			height: 0,
			constraints: Vec::new(),
			resizable: false,
		}
	}
}

impl WindowBuilder {
	pub fn with_application<T: Application>(mut self) -> Self {
		self.app_info = crate::make_app_info::<T>();
		self
	}

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

	pub fn resizable(mut self, resizable: bool) -> Self {
		self.resizable = resizable;
		self
	}

	pub fn build(self, display: &mut display::Manager) -> Result<Window, utility::AnyError> {
		optick::event!();
		log::info!(
			target: display::LOG,
			"Creating window \"{}\" ({}x{})",
			self.title.as_str(),
			self.width,
			self.height
		);
		let mut builder =
			display
				.video_subsystem()?
				.window(self.title.as_str(), self.width, self.height);
		builder.position_centered().vulkan();
		if self.resizable {
			builder.resizable();
		}
		Ok(display::Window::new(
			self.app_info,
			builder.build()?,
			self.constraints,
		)?)
	}
}

pub struct Window {
	graphics_queue_index: usize,

	// This is at the bottom to ensure that rust deallocates it last
	graphics_allocator: sync::Arc<graphics::alloc::Allocator>,
	logical_device: sync::Arc<logical::Device>,
	physical_device: sync::Arc<physical::Device>,
	surface: sync::Arc<Surface>,
	_vulkan: sync::Arc<instance::Instance>,
	internal: WinWrapper,
	_graphics_context: Context,
}

impl Window {
	pub fn new(
		app_info: AppInfo,
		sdl_window: sdl2::video::Window,
		constraints: Vec<physical::Constraint>,
	) -> Result<Window, utility::AnyError> {
		let graphics_context = Context::new()?;
		let internal = WinWrapper {
			internal: sdl_window,
		};
		let instance = instance::Info::default()
			.set_app_info(app_info)
			.set_window(&internal)
			.set_use_validation(is_vulkan_validation_enabled())
			.create_object(&graphics_context)?;
		let vulkan = sync::Arc::new(instance);
		let surface = sync::Arc::new(instance::Instance::create_surface(
			&graphics_context,
			&vulkan,
			&internal,
		)?);

		let physical_device = sync::Arc::new(Window::find_physical_device(
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
		let logical_device = sync::Arc::new(
			logical::Info::default()
				.add_extension("VK_KHR_swapchain")
				.set_validation_enabled(is_vulkan_validation_enabled())
				.add_queue(logical::DeviceQueue {
					queue_family_index: graphics_queue_index,
					priorities: vec![1.0],
				})
				.create_object(&vulkan, &physical_device)?,
		);

		let graphics_allocator = sync::Arc::new(graphics::alloc::Allocator::create(
			&vulkan,
			&physical_device,
			&logical_device,
		)?);

		Ok(Window {
			_graphics_context: graphics_context,
			internal,
			_vulkan: vulkan,
			graphics_allocator,
			surface,
			physical_device,
			logical_device,
			graphics_queue_index,
		})
	}

	fn find_physical_device(
		vulkan: &sync::Arc<instance::Instance>,
		surface: &sync::Arc<Surface>,
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
		render_pass_info: renderpass::Info,
	) -> utility::Result<sync::Arc<sync::RwLock<graphics::RenderChain>>> {
		optick::event!();
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

		Ok(sync::Arc::new(sync::RwLock::new(
			graphics::RenderChain::new(
				&self.physical_device,
				&self.logical_device,
				&self.graphics_allocator,
				graphics_queue,
				&self.surface,
				frame_count,
				render_pass_info,
			)?,
		)))
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

use crate::{
	graphics::{
		self,
		device::{logical, physical},
		flags, instance, renderpass, AppInfo, Context, Surface,
	},
	math::Vector,
	utility, window,
};
use std::sync;

fn is_vulkan_validation_enabled() -> bool {
	cfg!(debug_assertions)
}

pub struct Window {
	graphics_queue_index: usize,

	// This is at the bottom to ensure that rust deallocates it last
	render_chain: Option<sync::Arc<sync::RwLock<graphics::RenderChain>>>,
	render_pass_clear_color: Vector<f32, 4>,
	graphics_allocator: sync::Arc<graphics::alloc::Allocator>,
	logical_device: sync::Arc<logical::Device>,
	physical_device: sync::Arc<physical::Device>,
	surface: sync::Arc<Surface>,
	_vulkan: sync::Arc<instance::Instance>,
	internal: winit::window::Window,
	_graphics_context: Context,
}

impl Window {
	pub fn builder() -> window::Builder {
		window::Builder::default()
	}

	pub fn new(
		internal: winit::window::Window,
		app_info: AppInfo,
		constraints: Vec<physical::Constraint>,
		render_pass_clear_color: Vector<f32, 4>,
	) -> Result<Window, utility::AnyError> {
		let graphics_context = Context::new()?;
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
			target: window::LOG,
			"Window {:?} found physical device {}",
			internal.id(),
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
			render_pass_clear_color,
			render_chain: None,
			graphics_queue_index,
		})
	}

	pub fn unwrap(&self) -> &winit::window::Window {
		&self.internal
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
		&mut self,
		render_pass_info: renderpass::Info,
	) -> Result<sync::Arc<sync::RwLock<graphics::RenderChain>>, utility::AnyError> {
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

		let mut chain = graphics::RenderChain::new(
			&self.physical_device,
			&self.logical_device,
			&self.graphics_allocator,
			graphics_queue,
			&self.surface,
			frame_count,
			render_pass_info,
		)?;
		chain.add_clear_value(renderpass::ClearValue::Color(self.render_pass_clear_color));
		self.render_chain = Some(sync::Arc::new(sync::RwLock::new(chain)));

		Ok(self.render_chain().clone())
	}

	pub fn render_chain(&self) -> &sync::Arc<sync::RwLock<graphics::RenderChain>> {
		self.render_chain.as_ref().unwrap()
	}

	pub fn wait_until_idle(&self) -> Result<(), utility::AnyError> {
		Ok(self
			.render_chain()
			.read()
			.unwrap()
			.logical()
			.wait_until_idle()?)
	}
}

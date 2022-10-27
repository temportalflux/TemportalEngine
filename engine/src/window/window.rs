use crate::{
	graphics::{
		self,
		chain::DisplayResolution,
		device::{logical, physical, swapchain::khr},
		flags, instance,
		utility::HandledObject,
		utility::{BuildFromDevice, NameableBuilder},
		AppInfo, Chain, Context, Surface,
	},
	utility::{self},
	window,
};
use anyhow::Result;
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};
use std::sync::{self, Arc, RwLock};
use vulkan_rs::command;

fn is_vulkan_validation_enabled() -> bool {
	cfg!(debug_assertions)
}

pub struct Window {
	graphics_queue_index: usize,

	// These are at the bottom to ensure that rust deallocates them last
	chain: Option<Arc<RwLock<graphics::Chain>>>,
	graphics_allocator: Arc<graphics::alloc::Allocator>,
	logical_device: Arc<logical::Device>,
	physical_device: Arc<physical::Device>,
	surface: Arc<Surface>,
	_vulkan: Arc<instance::Instance>,
	_graphics_context: Context,
	internal: Arc<RwLock<winit::window::Window>>,
}

impl Window {
	pub fn builder() -> window::Builder {
		window::Builder::default()
	}

	pub fn new(
		internal: winit::window::Window,
		app_info: AppInfo,
		constraints: Vec<physical::Constraint>,
	) -> Result<Window> {
		let graphics_context = Context::new()?;
		let instance = instance::Info::default()
			.set_app_info(app_info)
			.set_window(internal.raw_display_handle())
			.set_use_validation(is_vulkan_validation_enabled())
			.create_object(&graphics_context)?;
		let vulkan = Arc::new(instance);
		let surface = Arc::new(instance::Instance::create_surface(
			&graphics_context,
			&vulkan,
			internal.raw_display_handle(),
			internal.raw_window_handle(),
		)?);

		let physical_device = Arc::new(Window::find_physical_device(
			&vulkan,
			&surface,
			constraints,
		)?);
		log::info!(
			target: window::LOG,
			"Found physical device {}",
			physical_device
		);

		let graphics_queue_index = physical_device
			.get_queue_index(flags::QueueFlags::GRAPHICS, true)
			.unwrap();
		let logical_device = Arc::new(
			logical::Info::default()
				.add_extension("VK_KHR_swapchain")
				.set_validation_enabled(is_vulkan_validation_enabled())
				.add_queue(logical::DeviceQueue {
					queue_family_index: graphics_queue_index,
					priorities: vec![1.0],
				})
				.with_name("GPU (firmware)".to_string())
				.create_object(&vulkan, &physical_device)?,
		);
		//logical_device.set_object_name_logged(&vulkan.create_name("Instance"));
		logical_device.set_object_name_logged(&physical_device.create_name("GPU (hardware)"));

		let graphics_allocator = Arc::new(graphics::alloc::Allocator::create(
			&vulkan,
			&physical_device,
			&logical_device,
		)?);

		Ok(Window {
			_graphics_context: graphics_context,
			internal: Arc::new(RwLock::new(internal)),
			_vulkan: vulkan,
			graphics_allocator,
			surface,
			physical_device,
			logical_device,
			chain: None,
			graphics_queue_index,
		})
	}

	pub fn unwrap(&self) -> sync::Weak<RwLock<winit::window::Window>> {
		Arc::downgrade(&self.internal)
	}

	pub fn position(&self) -> [f64; 2] {
		self.internal
			.read()
			.unwrap()
			.outer_position()
			.ok()
			.map(|pos| [pos.x as f64, pos.y as f64])
			.unwrap_or([0.0, 0.0])
	}

	pub fn read_size(&self) -> (winit::dpi::PhysicalSize<u32>, f64) {
		let handle = self.internal.read().unwrap();
		(handle.inner_size(), handle.scale_factor())
	}

	pub fn max_image_array_layers(&self) -> usize {
		self.physical_device.max_image_array_layers() as usize
	}

	fn find_physical_device(
		vulkan: &Arc<instance::Instance>,
		surface: &Arc<Surface>,
		constraints: Vec<physical::Constraint>,
	) -> anyhow::Result<physical::Device> {
		let mut constraints = constraints.clone();
		constraints.push(physical::Constraint::HasQueueFamily(
			flags::QueueFlags::GRAPHICS,
			/*requires_surface*/ true,
		));
		match instance::Instance::find_physical_device(&vulkan, &constraints, &surface) {
			Ok(device) => Ok(device),
			Err(failed_constraint) => Err(utility::error::FailedToFindPhysicalDevice(
				failed_constraint,
			))?,
		}
	}

	#[profiling::function]
	pub fn create_render_chain(&mut self) -> Result<&Arc<RwLock<Chain>>> {
		let surface_support = self.physical_device.query_surface_support();
		log::debug!("{surface_support:?}");

		let permitted_frame_count = surface_support.image_count_range();
		let frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start as usize),
			permitted_frame_count.end as usize,
		);

		let graphics_queue = Arc::new(logical::Device::create_queue(
			&self.logical_device,
			Some("Queue.Graphics".to_string()),
			self.graphics_queue_index,
		));

		self.chain = Some(Arc::new(RwLock::new(
			graphics::Chain::builder()
				.with_physical_device(self.physical_device.clone())
				.with_logical_device(self.logical_device.clone())
				.with_allocator(self.graphics_allocator.clone())
				.with_graphics_queue(graphics_queue)
				.with_transient_command_pool(Arc::new(
					command::Pool::builder()
						.with_name("CommandPool.Transient")
						.with_queue_family_index(self.graphics_queue_index)
						.with_flag(flags::CommandPoolCreate::TRANSIENT)
						.build(&self.logical_device)?,
				))
				.with_persistent_descriptor_pool(Arc::new(RwLock::new(
					graphics::descriptor::pool::Pool::builder()
						.with_name("DescriptorPool.Persistent")
						.with_total_set_count(100)
						.with_descriptor(flags::DescriptorKind::UNIFORM_BUFFER, 100)
						.with_descriptor(flags::DescriptorKind::COMBINED_IMAGE_SAMPLER, 100)
						.build(&self.logical_device)?,
				)))
				.with_swapchain(
					khr::Swapchain::builder()
						.with_name("Swapchain")
						.with_logical_device(&self.logical_device)
						.with_surface(&self.surface)
						.with_image_count(frame_count as u32)
						.with_image_format(flags::format::Format::B8G8R8A8_SRGB)
						.with_image_color_space(flags::ColorSpace::SRGB_NONLINEAR)
						.with_image_array_layer_count(1)
						.with_image_usage(flags::ImageUsageFlags::COLOR_ATTACHMENT)
						.with_image_sharing_mode(flags::SharingMode::EXCLUSIVE)
						.with_composite_alpha(flags::CompositeAlpha::OPAQUE)
						.with_is_clipped(true),
				)
				.with_resolution_provider(Arc::new(DisplayResolution))
				.build()?,
		)));

		Ok(self.graphics_chain())
	}

	pub fn graphics_chain(&self) -> &Arc<RwLock<graphics::Chain>> {
		&self.chain.as_ref().unwrap()
	}
}

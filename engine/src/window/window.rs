use crate::{
	graphics::{
		self,
		device::{logical, physical},
		flags, instance, renderpass,
		utility::HandledObject,
		AppInfo, Context, Surface,
	},
	math::nalgebra::Vector4,
	utility::{self},
	window,
};
use anyhow::Result;
use std::sync::{self, Arc, RwLock};

fn is_vulkan_validation_enabled() -> bool {
	cfg!(debug_assertions)
}

pub struct Window {
	graphics_queue_index: usize,

	// These are at the bottom to ensure that rust deallocates them last
	render_chain: Option<graphics::ArcRenderChain>,
	chain: Option<Arc<RwLock<graphics::Chain>>>,
	render_pass_clear_color: Vector4<f32>,
	graphics_allocator: sync::Arc<graphics::alloc::Allocator>,
	logical_device: sync::Arc<logical::Device>,
	physical_device: sync::Arc<physical::Device>,
	surface: sync::Arc<Surface>,
	_vulkan: sync::Arc<instance::Instance>,
	_graphics_context: Context,
	internal: sync::Arc<sync::RwLock<winit::window::Window>>,
}

impl Window {
	pub fn builder() -> window::Builder {
		window::Builder::default()
	}

	pub fn new(
		internal: winit::window::Window,
		app_info: AppInfo,
		constraints: Vec<physical::Constraint>,
		render_pass_clear_color: Vector4<f32>,
	) -> Result<Window> {
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
			"Found physical device {}",
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
				.with_name("GPU (firmware)".to_string())
				.create_object(&vulkan, &physical_device)?,
		);
		//logical_device.set_object_name_logged(&vulkan.create_name("Instance"));
		logical_device.set_object_name_logged(&physical_device.create_name("GPU (hardware)"));

		let graphics_allocator = sync::Arc::new(graphics::alloc::Allocator::create(
			&vulkan,
			&physical_device,
			&logical_device,
		)?);

		Ok(Window {
			_graphics_context: graphics_context,
			internal: sync::Arc::new(sync::RwLock::new(internal)),
			_vulkan: vulkan,
			graphics_allocator,
			surface,
			physical_device,
			logical_device,
			render_pass_clear_color,
			render_chain: None,
			chain: None,
			graphics_queue_index,
		})
	}

	pub fn unwrap(&self) -> sync::Weak<sync::RwLock<winit::window::Window>> {
		sync::Arc::downgrade(&self.internal)
	}

	pub fn read_size(&self) -> (winit::dpi::PhysicalSize<u32>, f64) {
		let handle = self.internal.read().unwrap();
		(handle.inner_size(), handle.scale_factor())
	}

	fn find_physical_device(
		vulkan: &sync::Arc<instance::Instance>,
		surface: &sync::Arc<Surface>,
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
	pub fn create_render_chain(
		&mut self,
		create_depth_image: bool,
	) -> Result<sync::Arc<sync::RwLock<graphics::RenderChain>>> {
		let permitted_frame_count = self
			.physical_device
			.query_surface_support()
			.image_count_range();
		let frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start as usize),
			permitted_frame_count.end as usize,
		);

		let graphics_queue = logical::Device::create_queue(
			&self.logical_device,
			Some("Queue.Graphics".to_string()),
			self.graphics_queue_index,
		);

		let mut chain = graphics::RenderChain::new(
			&self.physical_device,
			&self.logical_device,
			&self.graphics_allocator,
			graphics_queue,
			&self.surface,
			frame_count,
		)?;

		// Frame view
		chain.add_clear_value(renderpass::ClearValue::Color([
			self.render_pass_clear_color.x,
			self.render_pass_clear_color.y,
			self.render_pass_clear_color.z,
			self.render_pass_clear_color.w,
		]));
		// Color view
		chain.add_clear_value(renderpass::ClearValue::Color([
			self.render_pass_clear_color.x,
			self.render_pass_clear_color.y,
			self.render_pass_clear_color.z,
			self.render_pass_clear_color.w,
		]));
		// Depth buffer view
		chain.add_clear_value(renderpass::ClearValue::DepthStencil(1.0, 0));

		if create_depth_image {
			let tiling = flags::ImageTiling::OPTIMAL;
			let format_flags = flags::FormatFeatureFlags::DEPTH_STENCIL_ATTACHMENT;
			let format = self.physical_device.query_supported_image_formats(
				&vec![
					flags::format::Format::D32_SFLOAT,
					flags::format::Format::D32_SFLOAT_S8_UINT,
					flags::format::Format::D24_UNORM_S8_UINT,
				],
				tiling,
				format_flags,
			);
			if let Some(format) = format {
				chain.set_depth_format(format, tiling);
			} else {
				log::error!("Failed to find valid depth-buffer image format");
			}
		}

		self.chain = Some(Arc::new(RwLock::new(
			graphics::Chain::builder()
				.with_physical_device(self.physical_device.clone())
				.with_logical_device(self.logical_device.clone())
				.with_allocator(self.graphics_allocator.clone())
				.with_graphics_queue(Arc::new(logical::Device::create_queue(
					&self.logical_device,
					Some("Queue.Graphics".to_string()),
					self.graphics_queue_index,
				)))
				.with_transient_command_pool(chain.transient_command_pool().clone())
				.with_persistent_descriptor_pool(chain.persistent_descriptor_pool().clone())
				.build()?,
		)));
		self.render_chain = Some(sync::Arc::new(sync::RwLock::new(chain)));

		Ok(self.render_chain().clone())
	}

	pub fn render_chain(&self) -> &sync::Arc<sync::RwLock<graphics::RenderChain>> {
		self.render_chain.as_ref().unwrap()
	}

	pub fn graphics_chain(&self) -> &Arc<RwLock<graphics::Chain>> {
		&self.chain.as_ref().unwrap()
	}

	pub fn wait_until_idle(&self) -> Result<()> {
		Ok(self
			.render_chain()
			.read()
			.unwrap()
			.logical()
			.wait_until_idle()?)
	}
}

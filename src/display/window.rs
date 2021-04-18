use crate::{utility, Engine};
use sdl2;
use std::rc::Rc;
use temportal_graphics::{
	device::{logical, physical, swapchain},
	flags, image, instance, structs, Surface,
};

pub struct Window {
	frame_image_views: Vec<image::View>,
	frame_images: Vec<image::Image>,
	frame_count: u32,
	swapchain: Option<swapchain::Swapchain>,
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
			swapchain: None,
			frame_count: 0,
			frame_images: Vec::new(),
			frame_image_views: Vec::new(),
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

		let permitted_frame_count = self.physical().image_count_range();
		self.frame_count = std::cmp::min(
			std::cmp::max(3, permitted_frame_count.start),
			permitted_frame_count.end,
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
				.set_validation_enabled(self.engine.vulkan_validation_enabled)
				.add_queue(logical::DeviceQueue {
					queue_family_index: queue_idx,
					priorities: vec![1.0],
				})
				.create_object(&self.vulkan, &self.physical()),
		)?));
		Ok(())
	}

	pub fn logical(&self) -> &Rc<logical::Device> {
		&self.logical_device.as_ref().unwrap()
	}

	pub fn graphics_queue_index(&self) -> usize {
		self.graphics_queue_index.unwrap()
	}

	pub fn create_swapchain(&mut self) -> utility::Result<()> {
		assert!(self.frame_count > 0);

		self.frame_image_views.clear();
		self.frame_images.clear();

		self.swapchain = Some(utility::as_graphics_error(
			swapchain::Info::default()
				.set_image_count(self.frame_count)
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

		Ok(())
	}

	pub fn swapchain(&self) -> &swapchain::Swapchain {
		&self.swapchain.as_ref().unwrap()
	}

	pub fn frame_views(&self) -> &Vec<image::View> {
		&self.frame_image_views
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

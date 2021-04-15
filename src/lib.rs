extern crate sdl2;
extern crate shaderc;

use structopt::StructOpt;
use temportal_graphics::{
	self,
	device::{logical, physical, swapchain},
	flags::{
		self, ColorComponent, ColorSpace, CompositeAlpha, Format, ImageAspect, ImageUsageFlags,
		ImageViewType, PresentMode, QueueFlags, SharingMode,
	},
	image, instance, pipeline, shader,
	structs::ImageSubresourceRange,
	utility, AppInfo, Context,
};

#[path = "build/lib.rs"]
pub mod build;

#[path = "display/lib.rs"]
pub mod display;

#[path = "world/lib.rs"]
pub mod world;

#[derive(Debug, StructOpt)]
struct Opt {
	/// Use validation layers
	#[structopt(short, long)]
	validation_layers: bool,
	#[structopt(short, long)]
	build: bool,
}

fn vulkan_device_constraints() -> Vec<physical::Constraint> {
	use physical::Constraint::*;
	vec![
		HasQueueFamily(QueueFlags::GRAPHICS, /*requires_surface*/ true),
		HasSurfaceFormats(Format::B8G8R8A8_SRGB, ColorSpace::SRGB_NONLINEAR_KHR),
		HasExtension(String::from("VK_KHR_swapchain")),
		PrioritizedSet(
			vec![
				CanPresentWith(PresentMode::MAILBOX_KHR, Some(1)),
				CanPresentWith(PresentMode::FIFO_KHR, None),
			],
			false,
		),
		PrioritizedSet(
			vec![
				IsDeviceType(physical::Kind::DISCRETE_GPU, Some(100)),
				IsDeviceType(physical::Kind::INTEGRATED_GPU, Some(0)),
			],
			false,
		),
	]
}

pub fn run(_args: Vec<String>) -> Result<(), Box<dyn std::error::Error>> {
	let flags = Opt::from_args();
	if flags.build {
		return build::run();
	}

	let validation_enabled = flags.validation_layers;

	let display = display::EngineDisplay::new();
	let window = display::Window::new(&display, "Demo1", 800, 600);

	let ctx = Context::new()?;
	let app_info = AppInfo::new(&ctx)
		.engine("TemportalEngine", utility::make_version(0, 1, 0))
		.application("Demo1", utility::make_version(0, 1, 0));
	let instance = instance::Info::new()
		.set_app_info(app_info.clone())
		.set_window(&window)
		.set_use_validation(validation_enabled)
		.create_object(&ctx)?;
	let surface = instance.create_surface(&window);

	let constraints = vulkan_device_constraints();
	let physical_device = match instance.find_physical_device(&constraints, &surface) {
		Ok(device) => device,
		Err(failed_constraint) => match failed_constraint {
			None => panic!("Failed to find any rendering device (do you not have anyu GPUs?)"),
			Some(constraint) => panic!(
				"Failed to find physical device, failed on constraint {:?}",
				constraint
			),
		},
	};
	println!("Found physical device {}", physical_device);

	let logical_device = logical::Info::new()
		.add_extension("VK_KHR_swapchain")
		.set_validation_enabled(validation_enabled)
		.add_queue(logical::DeviceQueue {
			queue_family_index: physical_device
				.get_queue_index(QueueFlags::GRAPHICS, true)
				.unwrap(),
			priorities: vec![1.0],
		})
		.create_object(&instance, &physical_device);
	let permitted_frame_count = physical_device.image_count_range();
	let frame_count = std::cmp::min(
		std::cmp::max(3, permitted_frame_count.start),
		permitted_frame_count.end,
	);
	println!(
		"Will use {} frames, {:?}",
		frame_count, permitted_frame_count
	);

	let swapchain = swapchain::Info::new()
		.set_image_count(frame_count)
		.set_image_format(Format::B8G8R8A8_SRGB)
		.set_image_color_space(ColorSpace::SRGB_NONLINEAR_KHR)
		.set_image_extent(physical_device.image_extent())
		.set_image_array_layer_count(1)
		.set_image_usage(ImageUsageFlags::COLOR_ATTACHMENT)
		.set_image_sharing_mode(SharingMode::EXCLUSIVE)
		.set_pre_transform(physical_device.current_transform())
		.set_composite_alpha(CompositeAlpha::OPAQUE_KHR)
		.set_present_mode(physical_device.selected_present_mode)
		.set_is_clipped(true)
		.create_object(&logical_device, &surface);
	let frame_images = swapchain.get_images(&logical_device);
	println!("Found {} frame images", frame_images.len());

	let _frame_image_views = frame_images
		.iter()
		.map(|image| {
			image::ViewInfo::new()
				.set_view_type(ImageViewType::_2D)
				.set_format(Format::B8G8R8A8_SRGB)
				.set_subresource_range(ImageSubresourceRange {
					aspect_mask: ImageAspect::COLOR,
					base_mip_level: 0,
					level_count: 1,
					base_array_layer: 0,
					layer_count: 1,
				})
				.create_object(&logical_device, &image)
		})
		.collect::<Vec<_>>();

	let vert_shader = shader::Module::create(
		&logical_device,
		shader::Info {
			kind: flags::ShaderStageKind::VERTEX,
			entry_point: String::from("main"),
			bytes: include_bytes!("triangle.vert.spirv").to_vec(),
		},
	)?;
	let frag_shader = shader::Module::create(
		&logical_device,
		shader::Info {
			kind: flags::ShaderStageKind::FRAGMENT,
			entry_point: String::from("main"),
			bytes: include_bytes!("triangle.frag.spirv").to_vec(),
		},
	)?;

	let _pipeline = pipeline::Info::new()
		.add_shader(&vert_shader)
		.add_shader(&frag_shader)
		.set_viewport_state(
			pipeline::ViewportState::new()
				.add_viewport(utility::Viewport::new().set_size(physical_device.image_extent()))
				.add_scissor(utility::Scissor::new().set_size(physical_device.image_extent())),
		)
		.set_rasterization_state(pipeline::RasterizationState::new())
		.set_color_blending(pipeline::ColorBlendState::new().add_attachment(
			pipeline::ColorBlendAttachment {
				color_flags: ColorComponent::R
					| ColorComponent::G | ColorComponent::B
					| ColorComponent::A,
			},
		))
		.create_object(&logical_device);

	Ok(())
}

//use sdl2::event::Event;
//use sdl2::keyboard::Keycode;
//use sdl2::pixels::Color;
//use std::time::Duration;

// let mut canvas = window.window.into_canvas().build().unwrap();

// canvas.set_draw_color(Color::RGB(50, 0, 50));
// canvas.clear();
// canvas.present();

// // Game loop
// let mut event_pump = display.sdl.event_pump().unwrap();
// 'gameloop: loop {
// 	for event in event_pump.poll_iter() {
// 		match event {
// 			Event::Quit { .. } => break 'gameloop,
// 			Event::KeyDown {
// 				keycode: Some(Keycode::Escape),
// 				..
// 			} => break 'gameloop,
// 			_ => {}
// 		}
// 	}
// 	canvas.present();
// 	::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
// }

extern crate sdl2;

use structopt::StructOpt;
use temportal_graphics::{
	self,
	device::{logical, physical},
	instance, utility, AppInfo, ColorSpace, Context, Format, PresentMode, QueueFlags,
};

#[path = "display/lib.rs"]
pub mod display;

#[path = "world/lib.rs"]
pub mod world;

#[derive(Debug, StructOpt)]
struct Opt {
	/// Use validation layers
	#[structopt(short, long)]
	validation_layers: bool,
}

pub fn should_enable_validation() -> bool {
	Opt::from_args().validation_layers
}

fn vulkan_device_constraints() -> Vec<physical::Constraint> {
	use physical::Constraint::*;
	vec![
		HasQueueFamily(QueueFlags::GRAPHICS, /*requires_surface*/ true),
		HasSurfaceFormats(physical::SurfaceConstraint {
			formats: vec![Format::B8G8R8A8_SRGB],
			color_spaces: vec![ColorSpace::SRGB_NONLINEAR_KHR],
		}),
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
	let display = display::EngineDisplay::new();
	let window = display::Window::new(&display, "Demo1", 800, 600);

	let ctx = Context::new()?;
	let app_info = AppInfo::new(&ctx)
		.engine("TemportalEngine", utility::make_version(0, 1, 0))
		.application("Demo1", utility::make_version(0, 1, 0));
	let instance = instance::Info::new()
		.app_info(app_info.clone())
		.set_window(&window)
		.set_use_validation(should_enable_validation())
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

	let _logical_device = logical::Info::new()
		.add_extension("VK_KHR_swapchain")
		.set_validation_enabled(should_enable_validation())
		.add_queue(logical::DeviceQueue {
			queue_family_index: physical_device
				.get_queue_index(QueueFlags::GRAPHICS, true)
				.unwrap(),
			priorities: vec![1.0],
		})
		.create_object(&instance, &physical_device);
	let permitted_frame_count = physical_device.image_count_range();	
	let frame_count = std::cmp::min(std::cmp::max(3, permitted_frame_count.start), permitted_frame_count.end);
	println!("Will use {} frames, {:?}", frame_count, permitted_frame_count);

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

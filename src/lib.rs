extern crate sdl2;
extern crate shaderc;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;
use std::{cell::RefCell, rc::Rc};

use structopt::StructOpt;
use temportal_graphics::{self, renderpass, AppInfo, Context};
use temportal_math::Vector;

#[path = "asset/lib.rs"]
pub mod asset;

#[path = "build/lib.rs"]
pub mod build;

#[path = "display/lib.rs"]
pub mod display;

#[path = "graphics/lib.rs"]
pub mod graphics;

#[path = "world/lib.rs"]
pub mod world;

#[path = "utility/lib.rs"]
pub mod utility;

#[derive(Debug, StructOpt)]
struct Opt {
	/// Use validation layers
	#[structopt(short, long)]
	validation_layers: bool,
	#[structopt(short, long)]
	build: bool,
}

pub struct Engine {
	run_build_commandlet: bool,
	pub build_assets_callback: Option<build::BuildAssetsCallback>,

	vulkan_validation_enabled: bool,
	graphics_context: Context,
	app_info: AppInfo,

	pub assets: EngineAssets,

	quit_has_been_triggered: bool,
}

pub struct EngineAssets {
	pub types: asset::TypeRegistry,
	pub library: asset::Library,
	pub loader: asset::Loader,
}

impl Engine {
	pub fn new() -> Result<Engine, Box<dyn std::error::Error>> {
		use asset::Asset;

		let flags = Opt::from_args();
		let graphics_context = Context::new()?;
		let app_info = AppInfo::new(&graphics_context)
			.engine("TemportalEngine", utility::make_version(0, 1, 0));
		let mut engine = Engine {
			run_build_commandlet: flags.build,
			build_assets_callback: None,

			vulkan_validation_enabled: flags.validation_layers,
			graphics_context,
			app_info,

			assets: EngineAssets {
				types: asset::TypeRegistry::new(),
				library: asset::Library::new(),
				loader: asset::Loader::new(),
			},

			quit_has_been_triggered: false,
		};

		engine.assets.types.register(graphics::Shader::type_data());

		Ok(engine)
	}

	pub fn set_application(mut self, name: &str, version: u32) -> Self {
		self.app_info.set_application_info(name, version);
		self
	}

	pub fn app_info(&self) -> &AppInfo {
		&self.app_info
	}

	pub fn create_display_manager(engine: &Rc<RefCell<Self>>) -> utility::Result<display::Manager> {
		let mut manager = display::Manager::new(engine.clone())?;
		let weak_engine = Rc::downgrade(engine);
		manager.add_event_listener(weak_engine);
		Ok(manager)
	}

	pub fn is_build_instance(&self) -> bool {
		self.run_build_commandlet
	}

	pub fn build(&self) -> Result<(), Box<dyn std::error::Error>> {
		match self.build_assets_callback {
			Some(callback) => return build::run(callback),
			None => panic!("No valid assets callback provided"),
		}
	}
}

impl display::EventListener for Engine {
	fn on_event(&mut self, event: &sdl2::event::Event) {
		match event {
			Event::Quit { .. } => self.quit_has_been_triggered = true,
			Event::KeyDown {
				keycode: Some(Keycode::Escape),
				..
			} => self.quit_has_been_triggered = true,
			_ => {}
		}
	}
}

pub fn run(
	engine: &Rc<RefCell<Engine>>,
	display: &mut display::Manager,
	window: &mut Rc<RefCell<display::Window>>,
) -> Result<(), Box<dyn std::error::Error>> {
	window
		.borrow_mut()
		.add_clear_value(renderpass::ClearValue::Color(Vector::new([
			0.0, 0.0, 0.0, 1.0,
		])));
	window.borrow_mut().mark_commands_dirty();

	// Game loop
	while !engine.borrow().quit_has_been_triggered {
		display.poll_all_events()?;
		window.borrow_mut().render_frame()?;
		::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
	}

	window.borrow().logical().wait_until_idle()?;

	Ok(())
}

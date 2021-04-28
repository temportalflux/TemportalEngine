extern crate log;
extern crate sdl2;

use std::{cell::RefCell, rc::Rc};

#[path = "asset/lib.rs"]
pub mod asset;

#[path = "display/lib.rs"]
pub mod display;

#[path = "graphics/lib.rs"]
pub mod graphics;
use graphics::{AppInfo, Context};

pub use temportal_math as math;

#[path = "world/lib.rs"]
pub mod world;

#[path = "utility/lib.rs"]
pub mod utility;
use utility::AnyError;

pub mod logging;

#[path = "task/_.rs"]
pub mod task;

pub struct Engine {
	vulkan_validation_enabled: bool,
	graphics_context: Context,
	app_info: AppInfo,

	pub assets: EngineAssets,
}

pub struct EngineAssets {
	pub types: asset::TypeRegistry,
	pub library: asset::Library,
	pub loader: asset::Loader,
}

impl std::fmt::Debug for Engine {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "{:?}", self.app_info())
	}
}

impl Engine {
	pub fn new() -> Result<Rc<RefCell<Engine>>, AnyError> {
		let graphics_context = Context::new()?;
		let app_info = AppInfo::new(&graphics_context)
			.engine("TemportalEngine", utility::make_version(0, 1, 0));
		log::info!(target: app_info.engine_name(), "Initializing engine v{}", app_info.engine_version());
		let mut engine = Engine {
			vulkan_validation_enabled: cfg!(debug_assertions),
			graphics_context,
			app_info,

			assets: EngineAssets {
				types: asset::TypeRegistry::new(),
				library: asset::Library::new(),
				loader: asset::Loader::new(),
			},
		};

		engine.assets.types.register::<graphics::Shader>();
		engine.assets.types.register::<graphics::font::Font>();

		Ok(Rc::new(RefCell::new(engine)))
	}

	pub fn set_application(&mut self, name: &str, version: u32) {
		self.app_info.set_application_info(name, version);
	}

	pub fn app_info(&self) -> &AppInfo {
		&self.app_info
	}

	pub fn create_display_manager(
		engine: &Rc<RefCell<Self>>,
	) -> utility::Result<Rc<RefCell<display::Manager>>> {
		Ok(Rc::new(RefCell::new(display::Manager::new(
			engine.clone(),
		)?)))
	}
}

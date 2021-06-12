mod application;
pub mod asset;
pub mod audio;
pub use application::*;
pub mod ecs;
mod engine;
pub use engine::*;
pub mod graphics;
pub mod input;
pub mod logging;
pub use profiling;
pub mod math;
pub mod task;
pub mod ui;
pub mod utility;
pub mod window;
pub mod world;
pub use rand;
pub mod render;

use graphics::AppInfo;

pub fn manifest_location() -> &'static str {
	std::env!("CARGO_MANIFEST_DIR")
}

pub struct EngineApp();
impl Application for EngineApp {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn display_name() -> &'static str {
		"Temportal Engine"
	}
	fn location() -> &'static str {
		std::env!("CARGO_MANIFEST_DIR")
	}
	fn version() -> u32 {
		utility::make_version(
			std::env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
		)
	}
}

pub fn make_app_info<T: Application>() -> AppInfo {
	AppInfo::new()
		.engine(EngineApp::name(), EngineApp::version())
		.with_application(T::name(), T::version())
}

pub fn register_asset_types() {
	let mut locked = asset::TypeRegistry::get().write().unwrap();
	audio::register_asset_types(&mut locked);
	graphics::register_asset_types(&mut locked);
}

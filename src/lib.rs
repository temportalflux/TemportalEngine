mod application;
pub mod asset;
pub use application::*;
pub mod display;
pub mod ecs;
mod engine;
pub use engine::*;
pub mod graphics;
pub mod input;
pub mod logging;
pub use profiling;
pub use temportal_math as math;
pub mod task;
pub mod ui;
pub mod utility;
pub mod window;
pub mod world;

use graphics::AppInfo;

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
	locked.register::<graphics::Shader>();
	locked.register::<graphics::font::Font>();
	locked.register::<graphics::Texture>();
}

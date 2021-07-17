mod application;
pub use application::*;
pub mod asset;
pub mod audio;
pub mod ecs;
mod engine;
pub use engine::*;
pub mod graphics;
pub mod input;
pub mod logging;
pub mod math;
pub mod network;
pub use profiling;
pub mod task;
pub mod ui;
pub mod utility;
pub mod window;
pub mod world;
pub use rand;
pub mod render;

use graphics::AppInfo;

pub struct EngineApp();
impl Application for EngineApp {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn make_app_info<T: Application>() -> AppInfo {
	AppInfo::new()
		.engine(EngineApp::name(), EngineApp::version_int())
		.with_application(T::name(), T::version_int())
}

pub fn register_asset_types() {
	let mut locked = asset::TypeRegistry::get().write().unwrap();
	audio::register_asset_types(&mut locked);
	graphics::register_asset_types(&mut locked);
}

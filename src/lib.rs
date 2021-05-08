extern crate log;
extern crate sdl2;

#[path = "asset/_.rs"]
pub mod asset;

#[path = "display/_.rs"]
pub mod display;

mod application;
pub use application::*;

#[path = "graphics/_.rs"]
pub mod graphics;
use graphics::AppInfo;

pub mod logging;

pub use temportal_math as math;

#[path = "world/_.rs"]
pub mod world;

pub mod task;

#[path = "utility/_.rs"]
pub mod utility;

pub mod ecs;

pub fn make_app_info<T: Application>() -> AppInfo {
	AppInfo::new()
		.engine("TemportalEngine", utility::make_version(0, 1, 0))
		.with_application(T::name(), T::version())
}

pub fn register_asset_types() {
	let mut locked = asset::TypeRegistry::get().write().unwrap();
	locked.register::<graphics::Shader>();
	locked.register::<graphics::font::Font>();
	locked.register::<graphics::Texture>();
}

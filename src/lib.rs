mod application;
pub mod asset;
pub use application::*;
pub mod display;
pub mod ecs;
mod engine;
pub use engine::*;
pub mod graphics;
pub mod logging;
pub use temportal_math as math;
pub mod task;
pub mod utility;
pub mod window;
pub mod world;

use graphics::AppInfo;

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

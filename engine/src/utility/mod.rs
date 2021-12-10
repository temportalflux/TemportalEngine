mod error;
pub use error::*;

pub use temportal_engine_utilities::*;
pub use vulkan_rs::utility::make_version;

pub trait AToAny: 'static {
	fn as_any(&self) -> &dyn std::any::Any;
}
impl<T: 'static> AToAny for T {
	fn as_any(&self) -> &dyn std::any::Any {
		self
	}
}

pub mod singleton;

mod save_data;
pub use save_data::*;

pub mod kdl;

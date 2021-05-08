#[path = "error.rs"]
mod error;
pub use error::*;

pub use temportal_graphics::utility::make_version;

pub trait AToAny: 'static {
	fn as_any(&self) -> &dyn std::any::Any;
}
impl<T: 'static> AToAny for T {
	fn as_any(&self) -> &dyn std::any::Any {
		self
	}
}

pub mod singleton;

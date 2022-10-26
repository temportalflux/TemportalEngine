pub mod error;

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

pub fn spawn_thread<F, R, E>(target: &'static str, f: F) -> std::io::Result<std::thread::JoinHandle<()>>
where
	F: Fn() -> std::result::Result<R, E> + 'static + Send,
	E: std::fmt::Debug,
{
	std::thread::Builder::new()
		.name(target.to_owned())
		.spawn(move || {
			profiling::register_thread!();
			if let Err(err) = f() {
				log::error!(target: target, "{:?}", err);
			}
		})
}

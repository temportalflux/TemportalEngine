mod lib;
use lib::*;
fn main() -> engine::utility::VoidResult {
	#[cfg(feature = "profile")]
	{
		engine::profiling::optick::start_capture();
	}
	lib::run()?;
	#[cfg(feature = "profile")]
	{
		use engine::Application;
		engine::profiling::optick::stop_capture(UIDemo::name());
	}
	Ok(())
}

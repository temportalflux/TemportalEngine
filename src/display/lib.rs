#[path = "window.rs"]
mod window;

pub use window::Window;

pub struct EngineDisplay {
	sdl: sdl2::Sdl,
}

impl EngineDisplay {
	pub fn new() -> EngineDisplay {
		EngineDisplay {
			sdl: sdl2::init().unwrap(),
		}
	}

	pub fn video_subsystem(&self) -> sdl2::VideoSubsystem {
		self.sdl.video().unwrap()
	}
}

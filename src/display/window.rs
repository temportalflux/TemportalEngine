use crate::display::EngineDisplay;
use sdl2;

pub struct Window {
	window: sdl2::video::Window,
}

impl Window {
	pub fn new(display: &EngineDisplay, title: &str, width: u32, height: u32) -> Window {
		let mut builder = display.video_subsystem().window(title, width, height);
		let window = builder.position_centered().vulkan().build().unwrap();
		Window { window }
	}
}

unsafe impl raw_window_handle::HasRawWindowHandle for Window {
	fn raw_window_handle(&self) -> raw_window_handle::RawWindowHandle {
		self.window.raw_window_handle()
	}
}

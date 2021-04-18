use crate::{display::Window, utility, Engine};
use std::rc::Rc;

pub struct Manager {
	engine: Rc<Engine>,
	sdl: sdl2::Sdl,
}

impl Manager {
	pub fn new(engine: Rc<Engine>) -> utility::Result<Manager> {
		let sdl = utility::as_sdl_error(sdl2::init())?;
		Ok(Manager { engine, sdl })
	}

	pub fn video_subsystem(&self) -> utility::Result<sdl2::VideoSubsystem> {
		utility::as_sdl_error(self.sdl.video())
	}

	pub fn create_window(&self, title: &str, width: u32, height: u32) -> utility::Result<Window> {
		let mut builder = self.video_subsystem()?.window(title, width, height);
		let sdl_window = utility::as_window_error(builder.position_centered().vulkan().build())?;
		Window::new(&self.engine, sdl_window)
	}

	pub fn event_pump(&self) -> utility::Result<sdl2::EventPump> {
		utility::as_sdl_error(self.sdl.event_pump())
	}
}

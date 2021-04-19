use crate::{display, utility, Engine};
use std::{
	cell::RefCell,
	rc::{Rc, Weak},
};

pub struct Manager {
	engine: Rc<RefCell<Engine>>,
	sdl: sdl2::Sdl,
	event_listeners: Vec<Weak<RefCell<dyn display::EventListener>>>,
}

impl Manager {
	pub fn new(engine: Rc<RefCell<Engine>>) -> utility::Result<Manager> {
		let sdl = utility::as_sdl_error(sdl2::init())?;
		Ok(Manager {
			engine,
			sdl,
			event_listeners: Vec::new(),
		})
	}

	pub fn video_subsystem(&self) -> utility::Result<sdl2::VideoSubsystem> {
		utility::as_sdl_error(self.sdl.video())
	}

	pub fn create_window(
		&mut self,
		title: &str,
		width: u32,
		height: u32,
	) -> utility::Result<Rc<RefCell<display::Window>>> {
		let mut builder = self.video_subsystem()?.window(title, width, height);
		let sdl_window =
			utility::as_window_error(builder.position_centered().vulkan().resizable().build())?;
		let window = Rc::new(RefCell::new(display::Window::new(
			&self.engine,
			sdl_window,
		)?));
		let weak_ref = Rc::downgrade(&window);
		self.add_event_listener(weak_ref);
		Ok(window)
	}

	pub fn event_pump(&self) -> utility::Result<sdl2::EventPump> {
		utility::as_sdl_error(self.sdl.event_pump())
	}

	/// Adds a listener to list of listeners.
	/// Order is NOT preserved.
	pub fn add_event_listener(&mut self, listener: Weak<RefCell<dyn display::EventListener>>) {
		self.event_listeners.push(listener);
	}

	pub fn poll_all_events(&mut self) -> utility::Result<()> {
		for event in self.event_pump()?.poll_iter() {
			self.event_listeners
				.retain(|listener| listener.upgrade().is_some());
			for element in self.event_listeners.iter() {
				if element.upgrade().unwrap().borrow_mut().on_event(&event) {
					break; // event consumed, stop iterating over listeners and go to next event
				}
			}
		}
		Ok(())
	}
}

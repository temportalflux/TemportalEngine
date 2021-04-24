use crate::{display, utility::{self, VoidResult}, Engine};
use std::{
	cell::RefCell,
	rc::{Rc, Weak},
};

pub static LOG: &'static str = "display";

pub struct Manager {
	engine: Rc<RefCell<Engine>>,
	sdl: sdl2::Sdl,
	event_listeners: Vec<Weak<RefCell<dyn display::EventListener>>>,

	quit_has_been_triggered: bool,
}

impl Manager {
	pub fn new(engine: Rc<RefCell<Engine>>) -> utility::Result<Manager> {
		let sdl = utility::as_sdl_error(sdl2::init())?;
		Ok(Manager {
			engine,
			sdl,
			event_listeners: Vec::new(),
			quit_has_been_triggered: false,
		})
	}

	pub fn engine(&self) -> &Rc<RefCell<Engine>> {
		&self.engine
	}

	pub fn video_subsystem(&self) -> utility::Result<sdl2::VideoSubsystem> {
		utility::as_sdl_error(self.sdl.video())
	}

	pub fn create_sdl_window(
		&mut self,
		title: &str,
		width: u32,
		height: u32,
	) -> utility::Result<sdl2::video::Window> {
		log::info!(
			target: LOG,
			"Creating window \"{}\" ({}x{})",
			title,
			width,
			height
		);
		let mut builder = self.video_subsystem()?.window(title, width, height);
		utility::as_window_error(builder.position_centered().vulkan().resizable().build())
	}

	pub fn event_pump(&self) -> utility::Result<sdl2::EventPump> {
		utility::as_sdl_error(self.sdl.event_pump())
	}

	/// Adds a listener to list of listeners.
	/// Order is NOT preserved.
	pub fn add_event_listener(&mut self, listener: Weak<RefCell<dyn display::EventListener>>) {
		self.event_listeners.push(listener);
	}

	pub fn poll_all_events(&mut self) -> VoidResult {
		for event in self.event_pump()?.poll_iter() {
			self.event_listeners
				.retain(|listener| listener.strong_count() > 0);

			match event {
				sdl2::event::Event::Quit { .. } => {
					self.quit_has_been_triggered = true;
					continue;
				}
				_ => {}
			}

			for element in self.event_listeners.iter() {
				if element.upgrade().unwrap().borrow_mut().on_event(&event) {
					break; // event consumed, stop iterating over listeners and go to next event
				}
			}
		}
		Ok(())
	}

	pub fn should_quit(&self) -> bool {
		self.quit_has_been_triggered
	}
}

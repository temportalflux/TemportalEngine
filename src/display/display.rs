use crate::{
	display,
	utility::{self, VoidResult},
};
use std::{cell::RefCell, rc::Weak};

pub static LOG: &'static str = "display";

pub struct Manager {
	sdl: sdl2::Sdl,
	event_listeners: Vec<Weak<RefCell<dyn display::EventListener>>>,

	quit_has_been_triggered: bool,
}

impl Manager {
	pub fn new() -> utility::Result<Manager> {
		let sdl = sdl2::init().map_err(|e| utility::Error::Sdl(e))?;
		Ok(Manager {
			sdl,
			event_listeners: Vec::new(),
			quit_has_been_triggered: false,
		})
	}

	pub fn video_subsystem(&self) -> utility::Result<sdl2::VideoSubsystem> {
		self.sdl.video().map_err(|e| utility::Error::Sdl(e))
	}

	pub fn event_pump(&self) -> utility::Result<sdl2::EventPump> {
		self.sdl.event_pump().map_err(|e| utility::Error::Sdl(e))
	}

	/// Adds a listener to list of listeners.
	/// Order is NOT preserved.
	pub fn add_event_listener(&mut self, listener: Weak<RefCell<dyn display::EventListener>>) {
		self.event_listeners.push(listener);
	}

	pub fn poll_all_events(&mut self) -> VoidResult {
		optick::event!();
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

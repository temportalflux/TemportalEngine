use crate::{settings::Settings, ui};
use imgui::{self, im_str};

pub struct Simulation {
	id: String,
	is_open: bool,
	bring_to_front: bool,
}

impl Simulation {
	pub fn new() -> Simulation {
		let mut value = Simulation {
			id: "simulation".to_string(),
			is_open: false,
			bring_to_front: true,
		};
		let editor = crate::Editor::read();
		value.is_open = editor.settings.is_window_open(&value.id);
		value
	}

	pub fn open_or_bring_to_front(&mut self) {
		self.is_open = true;
		self.bring_to_front();
		self.save_open_state();
	}

	pub fn bring_to_front(&mut self) {
		self.bring_to_front = true;
	}

	fn save_open_state(&self) {
		let mut editor = crate::Editor::write();
		editor.settings.set_window_open(&self.id, self.is_open);
		if let Err(e) = editor.settings.save() {
			log::error!(target: "ui", "Failed to save editor settings, {}", e);
		}
	}
}

impl ui::Element for Simulation {
	fn render(&mut self, ui: &imgui::Ui) {
		if !self.is_open {
			return;
		}
		let was_open = self.is_open;
		imgui::Window::new(im_str!("Simulation"))
			.content_size([960.0, 540.0])
			.resizable(false)
			.opened(&mut self.is_open)
			.focused(self.bring_to_front)
			.build(&ui, || {
				//let viewport_size = ui.content_region_avail();
				//let tex = imgui::TextureId::new(0);
				//imgui::Image::new(tex, viewport_size).build(&ui);
			});
		self.bring_to_front = false;
		if self.is_open != was_open {
			self.save_open_state();
		}
	}
}

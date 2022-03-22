use engine::ui::egui::window::{ArcLockOpenWindowList, Window};

pub struct Simulation {
	open_list: Option<ArcLockOpenWindowList>,
}

impl Simulation {
	pub fn new() -> Self {
		Self { open_list: None }
	}
}

impl Window for Simulation {
	fn name(&self) -> &'static str {
		"simulation"
	}

	fn set_open_list(&mut self, open_list: ArcLockOpenWindowList) {
		self.open_list = Some(open_list);
	}

	fn show(&mut self, ctx: &egui::Context, is_open: &mut bool) {
		if !*is_open {
			return;
		}
		egui::Window::new(self.name())
			.open(is_open)
			.show(ctx, |ui| {
				ui.label("This is the simulation");
			});
	}
}

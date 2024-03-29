use engine::ui::egui::window::Window;

pub struct Simulation {
	display_name: String,
}

impl Simulation {
	pub fn new() -> Self {
		Self {
			display_name: "Simulation".to_owned(),
		}
	}
}

impl Window for Simulation {
	fn base_name(&self) -> &'static str {
		"simulation"
	}

	fn display_name(&self) -> &String {
		&self.display_name
	}

	fn show(&mut self, ctx: &egui::Context, id: egui::Id, is_open: &mut bool) {
		egui::Window::new(self.display_name())
			.id(id)
			.open(is_open)
			.show(ctx, |ui| {
				ui.label("This is the simulation");
			});
	}
}

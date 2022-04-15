use engine::ui::egui::window::Window;

pub struct AssetBrowser {
	display_name: String,
}

impl AssetBrowser {
	pub fn new() -> Self {
		Self {
			display_name: "Asset Browser".to_string(),
		}
	}
}

impl Window for AssetBrowser {
	fn base_name(&self) -> &'static str {
		"asset_browser"
	}

	fn display_name(&self) -> &String {
		&self.display_name
	}

	fn show(&mut self, ctx: &egui::Context, id: egui::Id, is_open: &mut bool) {
		egui::Window::new(self.display_name())
			.id(id)
			.open(is_open)
			.show(ctx, |ui| {
				ui.label("browser");
			});
	}
}

use crate::ui::{self, windows::OpenWindowList};
use std::sync::{Arc, RwLock};

pub struct Workspace {
	open_list: Arc<RwLock<OpenWindowList>>,
}

impl Workspace {
	pub fn new() -> Arc<RwLock<Self>> {
		let open_list = Arc::new(RwLock::new(OpenWindowList::new()));
		OpenWindowList::register_window(&open_list, ui::windows::Simulation::new());
		Arc::new(RwLock::new(Self {
			open_list,
		}))
	}
}

impl ui::Element for Workspace {
	fn render(&mut self, ctx: &egui::CtxRef) {
		egui::TopBottomPanel::top("menu_bar").show(ctx, |ui| {
			egui::menu::bar(ui, |ui| {
				egui::menu::menu(ui, "General", |ui| {
					let build = ui.button("Build").clicked();
					let rebuild = ui.button("Build (Force)").clicked();
					let package = ui.button("Package").clicked();
					let editor = crate::Editor::read();
					if build || rebuild {
						for module in editor.asset_modules.iter() {
							match module.build(editor.asset_manager(), rebuild) {
								Ok(_) => {}
								Err(e) => log::error!(target: "ui", "Failed to build... {:?}", e),
							}
						}
					}
					if package {
						for pak in editor.paks.iter() {
							if let Err(e) = pak.package() {
								log::error!(target: "ui", "Failed to package... {:?}", e);
							}
						}
					}
				});
				egui::menu::menu(ui, "Windows", |ui| {
					if let Ok(mut guard) = self.open_list.write() {
						guard.show_options(ui);
					}
				});
			});
		});
		if let Ok(mut guard) = self.open_list.write() {
			guard.show(ctx);
		}
		egui::CentralPanel::default().show(ctx, |ui| {
			ui.label("Hello World!");
			let _ = ui.button("this is a button");
		});
	}
}

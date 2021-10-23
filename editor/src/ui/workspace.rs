use crate::ui;
use std::sync::{Arc, RwLock};

pub struct Workspace {
	simulation: ui::windows::Simulation,
}

impl Workspace {
	pub fn new() -> Arc<RwLock<Workspace>> {
		Arc::new(RwLock::new(Workspace {
			simulation: ui::windows::Simulation::new(),
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
					if ui.button("Simulation").clicked() {
						/*
						self.simulation.open_or_bring_to_front();
						*/	
					}
				});
			});
		});
		egui::CentralPanel::default().show(ctx, |ui| {
			ui.label("Hello World!");
			let _ = ui.button("this is a button");
		});
		egui::Window::new("Test Window").show(ctx, |ui| {
			ui.label("window content goes here");
		});
		/*
		self.simulation.render(ui);
		*/
	}
}

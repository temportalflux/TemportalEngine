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
		egui::CentralPanel::default().show(ctx, |ui| {
			ui.label("Hello World!");
			ui.button("this is a button");
		});
		egui::Window::new("Test Window").show(ctx, |ui| {
			ui.label("window content goes here");
		});
		/*
		if let Some(bar) = ui.begin_main_menu_bar() {
			ui.menu(im_str!("General"), true, || {
				let build = imgui::MenuItem::new(im_str!("Build")).build(&ui);
				let rebuild = imgui::MenuItem::new(im_str!("Build (Force)")).build(&ui);
				let package = imgui::MenuItem::new(im_str!("Package")).build(&ui);
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
			ui.menu(im_str!("Windows"), true, || {
				if imgui::MenuItem::new(im_str!("Simulation")).build(&ui) {
					self.simulation.open_or_bring_to_front();
				}
			});
			bar.end(&ui);
		}
		self.simulation.render(ui);
		*/
	}
}

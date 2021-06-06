use crate::{asset, ui};
use imgui::{self, im_str};
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
	fn render(&mut self, ui: &imgui::Ui) {
		if let Some(bar) = ui.begin_main_menu_bar() {
			ui.menu(im_str!("General"), true, || {
				let build = imgui::MenuItem::new(im_str!("Build")).build(&ui);
				let rebuild = imgui::MenuItem::new(im_str!("Build (Force)")).build(&ui);
				let package = imgui::MenuItem::new(im_str!("Package")).build(&ui);
				let editor = crate::Editor::read();
				for app_module in editor.modules.iter() {
					if build || rebuild {
						match asset::build(
							editor.asset_manager(),
							&app_module.name,
							&app_module.location,
							rebuild,
						) {
							Ok(_) => {}
							Err(e) => log::error!(target: "ui", "Failed to build... {:?}", e),
						}
					}
					if package {
						match asset::package(
							&editor.settings,
							&app_module.name,
							&app_module.location,
							app_module.is_editor_only,
						) {
							Ok(_) => {}
							Err(e) => log::error!(target: "ui", "Failed to package... {:?}", e),
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
	}
}

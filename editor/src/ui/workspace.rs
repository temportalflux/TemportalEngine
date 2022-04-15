use crate::{ui, Editor};
use egui::{vec2, Align2};
use engine::ui::egui::Element;
use std::sync::{
	atomic::{AtomicBool, Ordering},
	Arc, RwLock,
};

pub struct Workspace {
	is_build_active: Arc<AtomicBool>,
	is_tasklist_open: bool,
}

impl Workspace {
	pub fn new() -> Arc<RwLock<Self>> {
		let editor = Editor::read();
		Arc::new(RwLock::new(Self {
			is_build_active: Arc::new(AtomicBool::new(false)),
			is_tasklist_open: false,
		}))
	}
}

impl Element for Workspace {
	fn render(&mut self, ctx: &egui::Context) {
		egui::TopBottomPanel::top("menu_bar").show(ctx, |ui| {
			egui::menu::bar(ui, |ui| {
				ui.menu_button("General", |ui| {
					// NOTE: This ui could be better.
					// Really what we care about is:
					// - Are assets being compiled/built?
					//   - if so, is it forced (i.e. purge the binaries before hand)?
					// - Should we package the assets (after building if we are building)?
					// - (eventually, but not yet supported) which modules? (this can be a set of options)
					//
					// So instead of a menu dropdown of multiple buttons, where only 1 is clicked per frame,
					// really we want some kind of dialog where:
					// - build and package are available in a multi-select dropdown (default is both)
					// - 'purge binaries' is only available if build is toggled
					// - multi-select dropdown with all of the modules attached to the editor
					//
					// This UX change has not been a priority so far since
					// assets are primarily built and packaged via CLI/commandlets.

					let build = ui.button("Build").clicked();
					let rebuild = ui.button("Build (Force)").clicked();
					let package = ui.button("Package").clicked();

					// If any of the buttons were clicked this frame, then the user wants to do a build operation
					if build || rebuild || package {
						// But we only allow new build operations if the old one has finished
						if !self.is_build_active.load(Ordering::Acquire) {
							// Clone the is-build-active flag so it can be set to false when the build is complete.
							let _ = Editor::build_and_package(
								build || rebuild,
								package,
								rebuild,
								Some(self.is_build_active.clone()),
							);
						}
					}
				});
				ui.menu_button("Create Window", |ui| {
					use ui::windows::*;
					let request = global_sender();
					if ui.button("Simulation").clicked() {
						request.open(Simulation::new());
					}
					if ui.button("Asset Browser").clicked() {
						request.open(AssetBrowser::new());
					}
				});
			});
		});
		egui::CentralPanel::default().show(ctx, |ui| {
			ui.label("Hello World!");
			let _ = ui.button("this is a button");

			egui::TopBottomPanel::bottom("footer").show(ctx, |ui| {
				ui.horizontal(|ui| {
					let task_count = engine::task::global_scopes().len();
					if task_count > 0 {
						ui.add(egui::Spinner::new());
						if ui.button(format!("{task_count} active tasks")).clicked() {
							self.is_tasklist_open = !self.is_tasklist_open;
						}
					} else {
						ui.label("No active tasks");
						if self.is_tasklist_open {
							self.is_tasklist_open = false;
						}
					}
				});
			});

			if self.is_tasklist_open {
				egui::Window::new("Task List")
					.open(&mut self.is_tasklist_open)
					.anchor(Align2::RIGHT_BOTTOM, vec2(0.0, -10.0))
					.title_bar(false)
					.resizable(false)
					.show(ctx, |ui| {
						ui.label("tasklist popout");
					});
			}
		});
		if let Ok(mut list) = ui::windows::global_list().write() {
			list.render(ctx);
		}
	}
}

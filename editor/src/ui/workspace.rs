use crate::{ui, Editor};
use engine::ui::egui::{window::OpenWindowList, Element};
use std::sync::{
	atomic::{AtomicBool, Ordering},
	Arc, RwLock,
};

pub struct Workspace {
	open_list: Arc<RwLock<OpenWindowList>>,
	is_build_active: Arc<AtomicBool>,
}

impl Workspace {
	pub fn new() -> Arc<RwLock<Self>> {
		let editor = Editor::read();
		let open_list = Arc::new(RwLock::new(
			OpenWindowList::new()
				.with_open_windows(editor.settings.get_open_window_list().clone().into_iter())
				.with_save_fn(|ids| {
					use engine::utility::SaveData;
					let mut editor = Editor::write();
					for (id, is_open) in ids.into_iter() {
						editor.settings.set_window_open(&id, is_open);
					}
					if let Err(e) = editor.settings.save() {
						log::error!(target: "ui", "Failed to save editor settings, {}", e);
					}
				}),
		));
		OpenWindowList::register_window(&open_list, ui::windows::Simulation::new());
		Arc::new(RwLock::new(Self {
			open_list,
			is_build_active: Arc::new(AtomicBool::new(false)),
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
							let async_is_active = self.is_build_active.clone();
							engine::task::spawn("editor".to_owned(), async move {
								assert!(!async_is_active.load(std::sync::atomic::Ordering::Acquire));
								let mut errors = Vec::new();

								// Build the assets (possibly forcibly).
								if build || rebuild {
									if let Err(errs) = Editor::build_assets(rebuild).await {
										errors = errs;
									}
								}

								// Pacakge the assets
								if package && errors.is_empty() {
									if let Err(errs) = Editor::package_assets().await {
										errors = errs;
									}
								}

								// There were no errors promoted above, so we are guarunteed to always
								// mark the flag as no-build-active when the operations have finished.
								async_is_active.store(false, Ordering::Relaxed);

								for error in errors.into_iter() {
									log::error!(target: "editor", "{error:?}");
								}
								Ok(())
							});
						}
					}
				});
				ui.menu_button("Windows", |ui| {
					if let Ok(mut guard) = self.open_list.write() {
						guard.show_options(ui);
					}
				});
			});
		});
		egui::CentralPanel::default().show(ctx, |ui| {
			ui.label("Hello World!");
			let _ = ui.button("this is a button");
			ui.add(egui::Spinner::new());
			ui.label("footer");

			//egui::TopBottomPanel::bottom("footer").show(ctx, |ui| {
			//	ui.with_layout(egui::Layout::right_to_left(), |ui| {
			//	});
			//});
		});
		if let Ok(mut guard) = self.open_list.write() {
			guard.render(ctx);
		}
	}
}

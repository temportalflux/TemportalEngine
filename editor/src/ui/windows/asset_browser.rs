use crate::{
	asset::{Module, SupportedFileTypes},
	ui::icons::Icon,
	Editor,
};
use egui::Vec2;
use engine::ui::egui::window::Window;
use std::{
	path::{Path, PathBuf},
	sync::Arc,
};

pub struct AssetBrowser {
	display_name: String,
	module_list: ModuleList,
	module_root: PathBuf,
	current_path: PathBuf,
	show_nonassets: bool,
}

impl AssetBrowser {
	pub fn new() -> Self {
		Self {
			display_name: "Asset Browser".to_string(),
			module_list: ModuleList::new(),
			module_root: PathBuf::new(),
			current_path: PathBuf::new(),
			show_nonassets: false,
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
			.resizable(true)
			.collapsible(true)
			.show(ctx, |ui| {
				if let Some(selected_module) = self.module_list.current() {
					if self.module_root != selected_module.assets_directory {
						self.module_root = selected_module.assets_directory.clone();
					}
				}
				if !self.current_path.starts_with(&self.module_root) {
					self.current_path = self.module_root.clone();
				}

				ui.horizontal(|ui| {
					self.module_list.show(ui);
					Breadcrumb::new(&self.module_root, &mut self.current_path).show(ui);
				});
				egui::ScrollArea::vertical().show(ui, |ui| {
					AssetExplorer::new(&mut self.current_path)
						.include_nonassets(self.show_nonassets)
						.show(ui);
				});
				ui.checkbox(&mut self.show_nonassets, "Show All Files");
			});
	}
}

struct ModuleList {
	items: Vec<Arc<Module>>,
	current_idx: usize,
}

impl ModuleList {
	fn new() -> Self {
		let items = Self::sorted_modules();
		Self {
			items,
			current_idx: 0,
		}
	}

	fn sorted_modules() -> Vec<Arc<Module>> {
		let mut asset_modules = Editor::read().asset_modules.clone();
		asset_modules.sort_by_key(|arc| arc.name.clone());
		asset_modules
	}

	fn show(&mut self, ui: &mut egui::Ui) {
		if let Some(current) = self.current() {
			egui::ComboBox::from_id_source("Module")
				.selected_text(current.name.clone())
				.show_ui(ui, |ui| {
					for (idx, item) in self.items.iter().enumerate() {
						ui.selectable_value(&mut self.current_idx, idx, item.name.clone());
					}
				});
		} else {
			ui.label("No modules");
		}
	}

	fn current(&self) -> Option<&Arc<Module>> {
		self.items.get(self.current_idx)
	}
}

struct Breadcrumb<'a> {
	root: &'a Path,
	current: &'a mut PathBuf,
}
impl<'a> Breadcrumb<'a> {
	fn new(root: &'a Path, current: &'a mut PathBuf) -> Self {
		Self { root, current }
	}

	fn get_subpaths(&self) -> Option<Vec<PathBuf>> {
		let relative_path = match self.current.strip_prefix(self.root) {
			Ok(path) => path,
			Err(_) => return None,
		};

		let mut subpaths = Vec::new();
		let mut subpath_builder = self.root.to_owned();
		for dir_item in relative_path.iter() {
			subpath_builder.push(dir_item);
			subpaths.push(subpath_builder.clone());
		}

		Some(subpaths)
	}

	fn show(&mut self, ui: &mut egui::Ui) {
		let subpaths = match self.get_subpaths() {
			Some(subpaths) => subpaths,
			None => {
				ui.label("InvalidPaths");
				return;
			}
		};

		ui.horizontal(|ui| {
			self.show_crumb(ui, "Module", &self.root);
			for subpath in subpaths.into_iter() {
				let name = subpath.file_stem().unwrap().to_str().unwrap();
				self.show_crumb(ui, name, &subpath);
			}
		});
	}

	fn show_crumb(&mut self, ui: &mut egui::Ui, name: &str, subpath: &Path) {
		let mut name_label = egui::Label::new(name);
		if subpath != self.current {
			name_label = name_label.sense(egui::Sense::click());
		}
		if ui.add(name_label).clicked() {
			*self.current = subpath.to_owned();
		}
		ui.label("/");
	}
}

struct AssetExplorer<'a> {
	current_path: &'a mut PathBuf,
	show_nonasset_files: bool,
}

impl<'a> AssetExplorer<'a> {
	fn new(current_path: &'a mut PathBuf) -> Self {
		Self {
			current_path,
			show_nonasset_files: false,
		}
	}

	fn include_nonassets(&mut self, include: bool) -> &mut Self {
		self.show_nonasset_files = include;
		self
	}

	fn show(&mut self, ui: &mut egui::Ui) {
		egui::Grid::new("explorer_grid")
			.striped(true)
			.num_columns(3)
			.show(ui, |ui| {
				ui.label("Name");
				ui.label("Type");
				ui.label("Extension");
				ui.end_row();

				self.show_grid_entries(ui);
			});
	}

	fn show_grid_entries(&mut self, ui: &mut egui::Ui) {
		if let Ok(entries) = std::fs::read_dir(&self.current_path) {
			for entry in entries {
				if let Ok(entry) = entry {
					self.show_grid_row(ui, entry.path());
				}
			}
		}
	}

	fn show_grid_row(&mut self, ui: &mut egui::Ui, path: PathBuf) {
		let is_dir = path.is_dir();
		// If this is not a directory and we shouldnt show non-asset files,
		// then filter out the file if its extension does not count as an asset.
		if !is_dir && !self.show_nonasset_files {
			let ext = path.extension().map(|ext| ext.to_str()).flatten();
			if SupportedFileTypes::parse_extension(ext).is_none() {
				return;
			}
		}

		ui.horizontal(|ui| {
			if is_dir {
				Icon::Folder.show(ui, Some(Vec2::new(20.0, 20.0)));
			}

			let r_name = ui.add({
				let name = path.file_stem().unwrap().to_str().unwrap();
				egui::Label::new(name).sense(egui::Sense::click())
			});
			if r_name.double_clicked() {
				if is_dir {
					*self.current_path = path.clone();
				} else {
					log::warn!("open file!");
				}
			}
		});

		if !is_dir {
			// could get asset type by reading the file synchronously via Manager, but that would be way to expense.
			// need the editor asset manager to keep tabs on all assets, so their asset types (and other data), can be queried without delays
			ui.label("NotCached");
		}

		if let Some(ext) = path.extension() {
			ui.label(ext.to_str().unwrap());
		}

		ui.end_row();
	}
}

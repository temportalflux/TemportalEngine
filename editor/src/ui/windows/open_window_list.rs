use std::{
	collections::{BTreeSet, HashMap},
	sync::{Arc, RwLock},
};

pub type ArcLockOpenWindowList = Arc<RwLock<OpenWindowList>>;

pub struct OpenWindowList {
	all_windows: HashMap<String, Box<dyn super::Window>>,
	open_window_names: BTreeSet<String>,
}

impl OpenWindowList {
	pub fn new() -> Self {
		let editor = crate::Editor::read();
		Self {
			all_windows: HashMap::new(),
			open_window_names: editor.settings.get_open_window_list().clone().into_iter().collect(),
		}
	}

	pub fn register_window<T>(arclock: &Arc<RwLock<Self>>, window: T)
	where
		T: 'static + super::Window,
	{
		if let Ok(mut guard) = arclock.write() {
			guard.add_window(window);
		}
	}

	pub fn add_window<T>(&mut self, window: T)
	where
		T: 'static + super::Window,
	{
		self.all_windows
			.insert(window.name().to_owned(), Box::new(window));
	}

	pub fn open(&mut self, id: String) {
		self.open_window_names.insert(id);
		self.save_open_state();
	}

	pub fn is_window_open(&self, id: &str) -> bool {
		self.open_window_names.contains(id)
	}

	fn save_open_state(&self) {
		use engine::utility::SaveData;
		let mut editor = crate::Editor::write();
		for (id, is_open) in self.all_windows.keys().map(|id| (id, self.is_window_open(&id))) {
			editor.settings.set_window_open(&id, is_open);
		}
		if let Err(e) = editor.settings.save() {
			log::error!(target: "ui", "Failed to save editor settings, {}", e);
		}
	}

	pub fn show_options(&mut self, ui: &mut egui::Ui) {
		let mut id_to_open: Option<String> = None;
		for (id, _window) in self.all_windows.iter() {
			let mut is_open = self.is_window_open(&id);
			if ui.checkbox(&mut is_open, &id).changed() {
				if is_open {
					id_to_open = Some(id.to_owned());
				}
			}
		}
		if let Some(id) = id_to_open {
			self.open(id);
		}
	}

	pub fn show(&mut self, ctx: &egui::CtxRef) {
		let mut open_ids = BTreeSet::new();
		for (id, window) in self.all_windows.iter_mut() {
			let mut is_open = self.open_window_names.contains(id);
			window.show(ctx, &mut is_open);
			if is_open {
				open_ids.insert(id.clone());
			}
		}
		if open_ids != self.open_window_names {
			self.open_window_names = open_ids;
			self.save_open_state();
		}
	}

}

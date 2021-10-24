use std::{
	collections::{BTreeSet, HashMap},
	sync::{Arc, RwLock},
};
use crate::ui::egui::Element;

pub type ArcLockOpenWindowList = Arc<RwLock<OpenWindowList>>;

pub struct OpenWindowList {
	all_windows: HashMap<String, Box<dyn super::Window>>,
	open_window_names: BTreeSet<String>,
	save_open_windows: Option<Box<dyn Fn(Vec<(String, bool)>)>>,
}

impl OpenWindowList {
	pub fn new() -> Self {
		Self {
			all_windows: HashMap::new(),
			open_window_names: BTreeSet::new(),
			save_open_windows: None,
		}
	}

	pub fn with_open_windows<T>(mut self, iter: T) -> Self
	where
		T: std::iter::Iterator<Item = String>,
	{
		self.open_window_names = iter.collect();
		self
	}

	pub fn with_save_fn<F>(mut self, callback: F) -> Self
	where
		F: 'static + Fn(Vec<(String, bool)>),
	{
		self.save_open_windows = Some(Box::new(callback));
		self
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
		if let Some(save_open_windows) = &self.save_open_windows {
			save_open_windows(
				self.all_windows
					.keys()
					.map(|id| (id.clone(), self.is_window_open(&id)))
					.collect(),
			);
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
}

impl Element for OpenWindowList {
	fn render(&mut self, ctx: &egui::CtxRef) {
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

use crate::ui::egui::Element;
use multimap::MultiMap;

pub type Receiver = crossbeam_channel::Receiver<Request>;

pub struct Sender(crossbeam_channel::Sender<Request>);
impl Sender {
	pub fn open<T>(&self, window: T) -> Result<(), crossbeam_channel::SendError<Request>>
	where
		T: Window + 'static + Send + Sync,
	{
		self.0.send(Request::Open(Box::new(window)))
	}
}

pub trait Window {
	fn base_name(&self) -> &'static str;
	fn display_name(&self) -> &String;
	fn show(&mut self, ctx: &egui::Context, id: egui::Id, is_open: &mut bool);
}

pub enum Request {
	Open(Box<dyn Window + 'static + Send + Sync>),
}
impl Request {
	pub fn channel() -> (Sender, Receiver) {
		let (sender, receiver) = crossbeam_channel::unbounded();
		(Sender(sender), receiver)
	}
}

pub struct Entry {
	id: egui::Id,
	window: Box<dyn Window + 'static + Send + Sync>,
}
impl Entry {
	fn show(&mut self, ctx: &egui::Context, is_open: &mut bool) {
		self.window.show(ctx, self.id, is_open);
	}
}

pub struct List {
	open_windows: Vec<Entry>,
	type_count: MultiMap<&'static str, usize>,
	discarded_ids: Vec<usize>,
	request_receiver: Option<Receiver>,
}

impl List {
	pub fn new(request_receiver: Option<Receiver>) -> Self {
		Self {
			open_windows: Vec::new(),
			type_count: MultiMap::new(),
			discarded_ids: Vec::new(),
			request_receiver,
		}
	}

	pub fn iter(&self) -> impl std::iter::Iterator<Item = &Entry> {
		self.open_windows.iter()
	}

	pub fn is_empty(&self) -> bool {
		self.open_windows.is_empty()
	}

	fn get_next_id(&mut self) -> usize {
		match self.discarded_ids.is_empty() {
			true => self.open_windows.len(),
			false => self.discarded_ids.remove(0),
		}
	}
}

impl Element for List {
	fn render(&mut self, ctx: &egui::Context) {
		let mut windows_to_open = Vec::new();
		if let Some(receiver) = &self.request_receiver {
			while let Ok(request) = receiver.try_recv() {
				match request {
					Request::Open(window) => {
						windows_to_open.push(window);
					}
				}
			}
		}
		for window in windows_to_open.into_iter() {
			let id = self.get_next_id();
			self.type_count.insert(window.base_name(), id);
			let id = egui::Id::new(format!("{}#{id}", window.base_name()));
			self.open_windows.push(Entry { window, id });
		}

		let mut i = 0;
		let mut count = self.open_windows.len();
		while i < count {
			let mut is_open = true;
			self.open_windows[i].show(ctx, &mut is_open);
			if is_open {
				i += 1;
			} else {
				let entry = self.open_windows.remove(i);
				if let Some(mut all_indices) = self.type_count.remove(entry.window.base_name()) {
					all_indices.retain(|&idx| idx != i);
					self.type_count
						.insert_many(entry.window.base_name(), all_indices);
				}
				count -= 1;
			}
		}
	}
}

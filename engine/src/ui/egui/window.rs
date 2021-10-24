mod list;
pub use list::*;

pub trait Window {
	fn name(&self) -> &'static str;
	fn set_open_list(&mut self, list: std::sync::Arc<std::sync::RwLock<OpenWindowList>>);
	fn show(&mut self, ctx: &egui::CtxRef, is_open: &mut bool);
}

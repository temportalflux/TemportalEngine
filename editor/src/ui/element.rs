use imgui;

pub trait Element {
	fn render(&mut self, ui: &imgui::Ui);
}

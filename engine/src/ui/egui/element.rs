pub trait Element {
	fn render(&mut self, ctx: &egui::Context);
}

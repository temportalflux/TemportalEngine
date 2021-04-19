pub trait EventListener {
	fn on_event(&mut self, event: &sdl2::event::Event) -> bool;
}

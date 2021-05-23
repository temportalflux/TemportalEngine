pub use winit::event::VirtualKeyCode as KeyCode;

#[derive(Debug)]
pub enum ButtonState {
	Pressed,
	Released,
}

#[derive(Debug)]
pub enum SystemEvent {
	MouseMove(/*pixels x*/ f64, /*pixels y*/ f64),
	MouseScroll(f32, f32),
	Axis(/*id*/ u32, /*value*/ f64),
	Button(/*id*/ u32, ButtonState),
	Key(KeyCode, ButtonState),
}

extern crate sdl2;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::pixels::Color;
use std::error::Error;
use std::time::Duration;
use temportal_math::*;

// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_right() -> Vector<f64, 3> {
	Vector::new([1.0, 0.0, 0.0])
}
pub fn global_up() -> Vector<f64, 3> {
	Vector::new([0.0, 1.0, 0.0])
}
pub fn global_forward() -> Vector<f64, 3> {
	Vector::new([0.0, 0.0, -1.0])
}

struct EngineDisplay {
	sdl: sdl2::Sdl,
}

impl EngineDisplay {

	pub fn video_subsystem(&self) -> sdl2::VideoSubsystem {
		self.sdl.video().unwrap()
	}

}

struct Window {
	window: sdl2::video::Window,
}

impl Window {

	pub fn new(display: &EngineDisplay, title: &str, width: u32, height: u32) -> Window {
		let mut builder = display.video_subsystem().window(title, width, height);
		let window = builder.position_centered().build().unwrap();
		Window { window }
	}

}

pub fn run(_args: Vec<String>) -> Result<(), Box<dyn Error>> {
	let display = EngineDisplay {
		sdl: sdl2::init().unwrap(),
	};

	let window = Window::new(&display, "Demo1", 800, 600);
	let mut canvas = window.window.into_canvas().build().unwrap();

	canvas.set_draw_color(Color::RGB(50, 0, 50));
	canvas.clear();
	canvas.present();

	// Game loop
	let mut event_pump = display.sdl.event_pump().unwrap();
	'gameloop: loop {
		for event in event_pump.poll_iter() {
			match event {
				Event::Quit { .. } => break 'gameloop,
				Event::KeyDown {
					keycode: Some(Keycode::Escape),
					..
				} => break 'gameloop,
				_ => {}
			}
		}
		canvas.present();
		::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
	}

	Ok(())
}

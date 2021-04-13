extern crate sdl2;

use std::error::Error;
use std::time::Duration;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::pixels::Color;

fn main() -> Result<(), Box<dyn Error>> {
	let sdl_context = sdl2::init().unwrap();
	let video_subsystem = sdl_context.video().unwrap();

	let window = video_subsystem
		.window("rust-sdl2 demo", 800, 600)
		.position_centered()
		.build()
		.unwrap();

	let mut canvas = window.into_canvas().build().unwrap();

	canvas.set_draw_color(Color::RGB(50, 0, 50));
	canvas.clear();
	canvas.present();

	// Game loop
	let mut event_pump = sdl_context.event_pump().unwrap();
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

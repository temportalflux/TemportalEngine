use crate::{asset, graphics, logging, task, utility::AnyError, Application};
use std::sync::{Arc, RwLock};
use winit::{
	event::Event,
	event_loop::{ControlFlow, EventLoop},
};

pub struct Engine {
	event_loop: EventLoop<()>,
}

impl Engine {
	pub fn new<T: Application>() -> Result<Engine, AnyError> {
		logging::init::<T>()?;
		task::initialize_system();
		crate::register_asset_types();
		asset::Library::scan_application::<T>()?;
		Ok(Engine {
			event_loop: EventLoop::new(),
		})
	}

	pub fn event_loop(&self) -> &EventLoop<()> {
		&self.event_loop
	}

	pub fn run(self, render_chain: Arc<RwLock<graphics::RenderChain>>) {
		let mut prev_frame_time = std::time::Instant::now();
		let mut delta_time = std::time::Duration::from_secs(0);
		self.event_loop.run(move |event, _, control_flow| {
			*control_flow = ControlFlow::Poll;
			match event {
				winit::event::Event::WindowEvent {
					window_id: _,
					event: winit::event::WindowEvent::CloseRequested,
				} => {
					*control_flow = winit::event_loop::ControlFlow::Exit;
				}
				Event::MainEventsCleared => {
					let frame_time = std::time::Instant::now();
					delta_time = frame_time - prev_frame_time;
					prev_frame_time = frame_time;

					task::watcher().poll();
					render_chain.write().unwrap().render_frame().unwrap();
				}
				Event::RedrawRequested(_) => {}
				Event::LoopDestroyed => {
					task::watcher().poll_until_empty();
				}
				_ => {}
			}
		});
	}
}

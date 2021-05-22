use crate::{asset, graphics, logging, task, utility::AnyError, Application, EngineApp};
use std::sync::{Arc, RwLock};
use winit::{
	event::Event,
	event_loop::{ControlFlow, EventLoop},
};

pub struct Engine {
	event_loop: Option<EventLoop<()>>,
	systems: Vec<Arc<RwLock<dyn EngineSystem>>>,
}

impl Engine {
	pub fn new<T: Application>() -> Result<Engine, AnyError> {
		logging::init::<T>()?;
		task::initialize_system();
		crate::register_asset_types();
		asset::Library::scan_application::<EngineApp>()?;
		asset::Library::scan_application::<T>()?;
		Ok(Engine {
			event_loop: Some(EventLoop::new()),
			systems: Vec::new(),
		})
	}

	pub fn event_loop(&self) -> &EventLoop<()> {
		self.event_loop.as_ref().unwrap()
	}

	pub fn add_system<T>(&mut self, system: &Arc<RwLock<T>>)
	where
		T: EngineSystem + 'static,
	{
		self.systems.push(system.clone());
	}

	pub fn run(self, render_chain: Arc<RwLock<graphics::RenderChain>>) {
		Self::run_engine(Arc::new(RwLock::new(self)), render_chain)
	}

	pub fn run_engine(engine: Arc<RwLock<Self>>, render_chain: Arc<RwLock<graphics::RenderChain>>) {
		let mut prev_frame_time = std::time::Instant::now();
		let mut prev_render_error = None;
		let event_loop = engine.write().unwrap().event_loop.take();
		event_loop.unwrap().run(move |event, _, control_flow| {
			profiling::scope!("run");
			*control_flow = ControlFlow::Poll;
			match event {
				winit::event::Event::WindowEvent {
					window_id: _,
					event: winit::event::WindowEvent::CloseRequested,
				} => {
					*control_flow = winit::event_loop::ControlFlow::Exit;
				}
				Event::MainEventsCleared => {
					profiling::scope!("update");
					let frame_time = std::time::Instant::now();
					task::watcher().poll();
					let delta_time = frame_time - prev_frame_time;
					{
						let systems = &mut engine.write().unwrap().systems;
						for system in systems.iter_mut() {
							system.write().unwrap().update(delta_time);
						}
					}
					{
						let mut chain_write = render_chain.write().unwrap();
						match chain_write.render_frame() {
							Ok(_) => prev_render_error = None,
							Err(error) => {
								if prev_render_error.is_none() {
									log::error!("Frame render failed {:?}", error);
								}
								prev_render_error = Some(error);
							}
						}
					}
					prev_frame_time = frame_time;
				}
				Event::RedrawRequested(_) => {}
				Event::LoopDestroyed => {
					task::watcher().poll_until_empty();
					render_chain
						.read()
						.unwrap()
						.logical()
						.wait_until_idle()
						.unwrap();
				}
				_ => {}
			}
		});
	}
}

pub trait EngineSystem {
	fn update(&mut self, delta_time: std::time::Duration);
}

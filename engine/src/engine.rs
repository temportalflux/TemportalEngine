use crate::{
	asset, audio, input, network, task,
	utility::{AnyError, VoidResult},
};
use std::sync::{
	atomic::{self, AtomicBool},
	Arc, RwLock, RwLockWriteGuard,
};
use winit::event_loop::EventLoop;

pub struct Engine {
	event_loop: Option<EventLoop<()>>,
	winit_listeners: Vec<Arc<RwLock<dyn WinitEventListener>>>,
	systems: Vec<Arc<RwLock<dyn EngineSystem>>>,
	window: Option<crate::window::Window>,
}

impl Engine {
	pub fn new() -> Result<Self, AnyError> {
		task::initialize_system();
		crate::register_asset_types();
		audio::System::initialize();
		Ok(Self {
			event_loop: Some(EventLoop::new()),
			winit_listeners: Vec::new(),
			systems: Vec::new(),
			window: None,
		})
	}

	/// Scans the engine pak file and any pak file names provided.
	pub fn scan_paks(&self) -> VoidResult {
		let mut library = asset::Library::write();
		library.scan_pak_directory()?;
		Ok(())
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

	pub fn add_winit_listener<T>(&mut self, system: &Arc<RwLock<T>>)
	where
		T: WinitEventListener + 'static,
	{
		self.winit_listeners.push(system.clone());
	}

	pub fn set_window(&mut self, window: crate::window::Window) -> &mut crate::window::Window {
		self.window = Some(window);
		self.window.as_mut().unwrap()
	}

	pub fn window(&self) -> Option<&crate::window::Window> {
		self.window.as_ref()
	}

	pub fn render_chain(&self) -> Option<&crate::graphics::ArcRenderChain> {
		self.window.as_ref().map(|win| win.render_chain())
	}

	pub fn render_chain_write(&self) -> Option<RwLockWriteGuard<crate::graphics::RenderChain>> {
		self.render_chain()
			.map(|chain| chain.write().ok())
			.flatten()
	}

	pub fn into_arclock(self) -> Arc<RwLock<Self>> {
		Arc::new(RwLock::new(self))
	}

	pub fn run<F>(engine: Arc<RwLock<Self>>, on_complete: F) -> !
	where
		F: 'static + Fn() -> (),
	{
		let terminate_signal = Arc::new(AtomicBool::new(false));
		let _ = signal_hook::flag::register(signal_hook::consts::SIGINT, terminate_signal.clone());

		let mut prev_frame_time = std::time::Instant::now();
		let mut prev_render_error = None;
		let event_loop = engine.write().unwrap().event_loop.take();
		let mut engine_has_focus = true;
		event_loop.unwrap().run(move |event, _, control_flow| {
			use winit::{event::*, event_loop::*};
			profiling::scope!("run");
			*control_flow = ControlFlow::Poll;
			if terminate_signal.load(atomic::Ordering::Relaxed) {
				*control_flow = ControlFlow::Exit;
				return;
			}
			if engine_has_focus {
				if let Ok((source, input_event)) = input::winit::parse_winit_event(&event) {
					input::write().send_event(source, input_event);
				}
				{
					let systems = &mut engine.write().unwrap().winit_listeners;
					for system in systems.iter_mut() {
						system.write().unwrap().on_event(&event);
					}
				}
			}
			match event {
				Event::WindowEvent {
					window_id: _,
					event: WindowEvent::CloseRequested,
				} => {
					*control_flow = ControlFlow::Exit;
				}
				Event::WindowEvent {
					window_id: _,
					event: WindowEvent::Focused(has_focus),
				} => {
					engine_has_focus = has_focus;
				}
				Event::MainEventsCleared => {
					profiling::scope!("update");
					let frame_time = std::time::Instant::now();
					task::watcher().poll();
					if engine_has_focus {
						input::write().update();
					}
					let delta_time = frame_time - prev_frame_time;
					{
						let _ = network::Network::process();
						audio::System::write().unwrap().update(delta_time);
						let systems = &mut engine.write().unwrap().systems;
						for system in systems.iter_mut() {
							system.write().unwrap().update(delta_time);
						}
					}
					if let Ok(eng) = engine.read() {
						if let Some(mut chain_write) = eng.render_chain_write() {
							match (*chain_write).render_frame() {
								Ok(_) => prev_render_error = None,
								Err(error) => {
									if prev_render_error.is_none() {
										log::error!("Frame render failed {:?}", error);
									}
									prev_render_error = Some(error);
									*control_flow = winit::event_loop::ControlFlow::Exit;
								}
							}
						}
					}
					prev_frame_time = frame_time;
				}
				Event::RedrawRequested(_) => {}
				Event::LoopDestroyed => {
					log::info!(target: "engine", "Engine loop complete");
					task::watcher().poll_until_empty();
					if let Ok(eng) = engine.read() {
						if let Some(chain_read) =
							eng.render_chain().map(|chain| chain.read().unwrap())
						{
							chain_read.logical().wait_until_idle().unwrap();
						}
					}
					on_complete();
				}
				_ => {}
			}
		})
	}
}

pub trait WinitEventListener {
	fn on_event(&mut self, event: &winit::event::Event<()>);
}

pub trait EngineSystem {
	fn update(&mut self, delta_time: std::time::Duration);
}

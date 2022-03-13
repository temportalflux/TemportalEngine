use crate::{asset, audio, input, task};
use anyhow::Result;
use std::sync::{
	atomic::{self, AtomicBool},
	Arc, RwLock, Weak,
};
use winit::event_loop::EventLoop;

pub struct Engine {
	winit_listeners: Vec<Arc<RwLock<dyn WinitEventListener + Send + Sync>>>,
	systems: Vec<Arc<RwLock<dyn EngineSystem + Send + Sync>>>,
	weak_systems: Vec<Weak<RwLock<dyn EngineSystem + Send + Sync>>>,
}

impl Engine {
	pub fn new() -> Result<Self> {
		task::initialize_system();
		audio::System::initialize();
		Ok(Self {
			winit_listeners: Vec::new(),
			systems: Vec::new(),
			weak_systems: Vec::new(),
		})
	}

	/// Scans the engine pak file and any pak file names provided.
	pub fn scan_paks(&self) -> Result<()> {
		let mut library = asset::Library::write();
		library.scan_pak_directory()?;
		Ok(())
	}

	pub fn add_system<T>(&mut self, system: Arc<RwLock<T>>)
	where
		T: EngineSystem + 'static + Send + Sync,
	{
		self.systems.push(system);
	}

	pub fn add_weak_system<T>(&mut self, system: Weak<RwLock<T>>)
	where
		T: EngineSystem + 'static + Send + Sync,
	{
		self.weak_systems.push(system);
	}

	pub fn add_winit_listener<T>(&mut self, system: &Arc<RwLock<T>>)
	where
		T: WinitEventListener + 'static + Send + Sync,
	{
		self.winit_listeners.push(system.clone());
	}

	pub fn into_arclock(self) -> Arc<RwLock<Self>> {
		Arc::new(RwLock::new(self))
	}

	fn singleton() -> &'static mut std::mem::MaybeUninit<Arc<RwLock<Self>>> {
		use std::mem::MaybeUninit;
		static mut INSTANCE: MaybeUninit<Arc<RwLock<Engine>>> = MaybeUninit::uninit();
		unsafe { &mut INSTANCE }
	}

	pub fn set(engine: Arc<RwLock<Self>>) {
		Self::singleton().write(engine);
	}

	pub fn get() -> &'static Arc<RwLock<Self>> {
		unsafe { &*Self::singleton().as_ptr() }
	}

	pub fn run<TRuntime>(engine: Arc<RwLock<Self>>, mut runtime: TRuntime) -> !
	where
		TRuntime: crate::Runtime + 'static,
	{
		Self::set(engine.clone());
		let terminate_signal = Arc::new(AtomicBool::new(false));
		let _ = signal_hook::flag::register(signal_hook::consts::SIGINT, terminate_signal.clone());

		/* the holy trinity of fixed timestep articles:
		- https://gafferongames.com/post/fix_your_timestep/ (the one most people reference)
		- https://gameprogrammingpatterns.com/game-loop.html (the same concept, but more clearly explained)
		- https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
			(various techniques for making a fixed timestep more robust, and some explanations of why non-fixed timestep can be bad)
		*/
		let mut prev_frame_time = std::time::Instant::now();
		let mut prev_render_error = None;

		let event_loop = EventLoop::new();
		let create_display_result = runtime.create_display(&engine, &event_loop);

		let mut engine_has_focus = true;
		event_loop.run(move |event, _, control_flow| {
			use winit::{event::*, event_loop::*};
			profiling::scope!("run");
			*control_flow = ControlFlow::Poll;

			if let Err(err) = &create_display_result {
				log::error!(target: "main", "{:?}", err);
				*control_flow = ControlFlow::Exit;
				return;
			}

			if terminate_signal.load(atomic::Ordering::Relaxed) {
				*control_flow = ControlFlow::Exit;
				return;
			}

			if engine_has_focus {
				profiling::scope!("parse-input");
				if let Ok(event) = input::winit::parse_winit_event(&event) {
					input::send_event(event);
				}
				{
					let engine = engine.read().unwrap();
					for system in engine.winit_listeners.iter() {
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
					task::poll_until_empty();

					if engine_has_focus {
						profiling::scope!("input");
						let users = if let Ok(mut cache) = input::device_cache().write() {
							cache.update();
							cache.users()
						} else {
							vec![]
						};

						let now = std::time::Instant::now();
						for weak_user in users.into_iter() {
							if let Some(arc_user) = weak_user.upgrade() {
								if let Ok(mut user) = arc_user.write() {
									user.update(&now);
								}
							}
						}
					}
					let delta_time = frame_time - prev_frame_time;
					{
						let mut engine = engine.write().unwrap();
						engine.weak_systems.retain(|sys| sys.strong_count() > 0);
					}
					{
						profiling::scope!("audio");
						audio::System::write()
							.unwrap()
							.update(delta_time, engine_has_focus);
					}
					{
						profiling::scope!(
							"update-systems",
							&format!("Δt={:.3}", delta_time.as_secs_f32())
						);
						let systems = {
							let engine = engine.read().unwrap();
							let strong = engine.systems.iter().cloned();
							let weak = engine.weak_systems.iter().filter_map(|weak| weak.upgrade());
							strong.chain(weak).collect::<Vec<_>>()
						};
						for system in systems.into_iter() {
							system.write().unwrap().update(delta_time, engine_has_focus);
						}
					}

					if let Some(arc_chain) = runtime.get_display_chain() {
						if let Ok(mut chain) = arc_chain.write() {
							profiling::scope!("render");
							match chain.render_frame() {
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
				Event::RedrawRequested(_) => {
					profiling::scope!("redraw");
				}
				Event::LoopDestroyed => {
					profiling::scope!("shutdown");
					log::info!(target: "engine", "Engine loop complete");
					task::poll_until_empty();
					if let Some(arc_chain) = runtime.get_display_chain() {
						if let Ok(chain) = arc_chain.read() {
							if let Ok(logical) = chain.logical() {
								logical.wait_until_idle().unwrap();
							}
						}
					}
					if let Ok(mut engine) = engine.write() {
						engine.winit_listeners.clear();
						engine.weak_systems.clear();
						engine.systems.clear();
					}
					runtime.on_event_loop_complete();
				}
				_ => {
					profiling::scope!("unknown");
				}
			}
		})
	}
}

pub trait WinitEventListener {
	fn on_event(&mut self, event: &winit::event::Event<()>);
}

pub trait EngineSystem {
	fn update(&mut self, delta_time: std::time::Duration, has_focus: bool);
}

use std::{
	path::PathBuf,
	sync::{Arc, RwLock},
};

mod application;
pub use application::*;
pub mod asset;
pub mod audio;
pub mod ecs;
mod engine;
pub use engine::*;
pub mod channels {
	pub mod future {
		pub use async_channel::*;
		pub type Pair<T> = (Sender<T>, Receiver<T>);
	}
	pub use bus as broadcast;
	pub use crossbeam_channel as mpsc;
}
pub mod graphics;
pub mod input;
pub mod logging;
pub mod math;
pub use profiling;
pub mod task;
pub mod ui;
pub mod utility;
pub mod window;
pub mod world;
pub use rand;
pub mod render;

use graphics::AppInfo;
pub use winit::event_loop::EventLoop;

use crate::{graphics::Chain, task::PinFutureResultLifetime};

pub struct EngineApp();
impl Application for EngineApp {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn make_app_info<T: Application>() -> AppInfo {
	AppInfo::new()
		.engine(EngineApp::name(), EngineApp::version_int())
		.with_application(T::name(), T::version_int())
}

pub trait Runtime {
	fn logging_path() -> PathBuf;

	fn register_asset_types() {
		let mut registry = asset::TypeRegistry::get().write().unwrap();
		crate::register_asset_types(&mut registry);
	}

	fn initialize<'a>(&'a self, engine: Arc<RwLock<Engine>>) -> PinFutureResultLifetime<'a, bool>;

	fn create_display(
		&mut self,
		_engine: &Arc<RwLock<Engine>>,
		_event_loop: &EventLoop<()>,
	) -> anyhow::Result<()> {
		Ok(())
	}

	fn get_display_chain(&self) -> Option<&Arc<RwLock<Chain>>> {
		None
	}

	fn on_event_loop_complete(&self) {}
}

pub fn register_asset_types(registry: &mut asset::TypeRegistry) {
	audio::register_asset_types(registry);
	graphics::register_asset_types(registry);
}

fn create_task_thread_name(idx: usize) -> String {
	static NAMES: [&'static str; 16] = [
		"alpha", "bravo", "canon", "delta", "ephor", "flump", "gnome", "hedge", "igloo", "julep",
		"knoll", "liege", "magic", "novel", "omega", "panda",
	];
	if idx < NAMES.len() {
		NAMES[idx].to_owned()
	} else {
		format!("task-worker:{}", idx)
	}
}

pub fn register_thread() {
	// Starting the Tracy client is necessary before any invoking any of its APIs
	#[cfg(feature = "profile")]
	tracy_client::Client::start();
	profiling::register_thread!();
}

pub fn run<TRuntime>(runtime: TRuntime)
where
	TRuntime: Runtime + 'static,
{
	let async_runtime = {
		profiling::scope!("setup-runtime");
		let thread_count = num_cpus::get();
		let mut builder = tokio::runtime::Builder::new_multi_thread();
		builder.enable_all();
		builder.worker_threads(thread_count);
		builder.thread_name_fn(|| {
			use std::sync::atomic::{AtomicUsize, Ordering};
			static THREAD_ID: AtomicUsize = AtomicUsize::new(0);
			let id = THREAD_ID.fetch_add(1, Ordering::SeqCst);
			create_task_thread_name(id)
		});
		let async_runtime = builder.build().unwrap();
		// Thread Registration
		{
			profiling::scope!("spawn-registration-tasks");
			let arclock = Arc::new(RwLock::new(0));
			for _ in 0..thread_count {
				let thread_counter = arclock.clone();
				async_runtime.spawn(async move {
					register_worker_thread(thread_counter, thread_count);
				});
			}
		}
		async_runtime
	};
	async_runtime.block_on(async {
		if let Err(err) = crate::execute_runtime(runtime).await {
			log::error!(target: "main", "Runtime error: {:?}", err);
		}
	});
}

fn register_worker_thread(thread_counter: Arc<RwLock<usize>>, thread_count: usize) {
	profiling::register_thread!();
	profiling::scope!("register_worker_thread");
	static THREAD_DELAY: std::time::Duration = std::time::Duration::from_millis(1);
	if let Ok(mut counter) = thread_counter.write() {
		*counter += 1;
	}
	// Block the worker thread until all threads have been registered.
	while *thread_counter.read().unwrap() < thread_count {
		std::thread::sleep(THREAD_DELAY);
	}
}

async fn execute_runtime<TRuntime>(runtime: TRuntime) -> anyhow::Result<()>
where
	TRuntime: Runtime + 'static,
{
	use anyhow::Context;
	logging::init(&TRuntime::logging_path()).context("initialize logging")?;
	TRuntime::register_asset_types();

	let engine = Engine::new().context("create engine")?.into_arclock();
	Engine::set(engine.clone());
	if runtime
		.initialize(engine.clone())
		.await
		.context("initialize runtime")?
	{
		engine::Engine::run(engine, runtime)
	} else {
		Ok(())
	}
}

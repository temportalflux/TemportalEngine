use std::sync::{Arc, RwLock};

mod application;
pub use application::*;
pub mod asset;
pub mod audio;
pub mod ecs;
mod engine;
pub use engine::*;
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

pub fn register_asset_types() {
	let mut locked = asset::TypeRegistry::get().write().unwrap();
	audio::register_asset_types(&mut locked);
	graphics::register_asset_types(&mut locked);
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

pub fn run<F>(f: F)
where
	F: Fn() -> anyhow::Result<()>,
{
	profiling::register_thread!();
	let runtime = {
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
		let runtime = builder.build().unwrap();
		// Thread Registration
		{
			profiling::scope!("spawn-registration-tasks");
			let arclock = Arc::new(RwLock::new(0));
			for _ in 0..thread_count {
				let thread_counter = arclock.clone();
				runtime.spawn(async move {
					register_worker_thread(thread_counter, thread_count);
				});
			}
		}
		runtime
	};
	runtime.block_on(async {
		if let Err(err) = f() {
			log::error!(target: "main", "{}", err);
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

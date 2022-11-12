use engine::{
	graphics::{chain::procedure::DefaultProcedure, Chain},
	task::PinFutureResultLifetime,
	window::Window,
	Application, Engine, EventLoop,
};
use std::{
	path::PathBuf,
	sync::{Arc, RwLock},
};

#[path = "renderer.rs"]
mod renderer;
#[path = "vertex.rs"]
mod vertex;
pub use vertex::*;

pub struct TriangleDemo();
impl Application for TriangleDemo {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub struct Runtime {
	renderer: Option<Arc<RwLock<renderer::Triangle>>>,
	window: Option<Window>,
}
impl Runtime {
	pub fn new() -> Self {
		Self {
			window: None,
			renderer: None,
		}
	}
}
impl engine::Runtime for Runtime {
	fn logging_path() -> PathBuf {
		engine::logging::default_path(TriangleDemo::name(), None)
	}

	fn initialize<'a>(&'a self, _engine: Arc<RwLock<Engine>>) -> PinFutureResultLifetime<'a, bool> {
		Box::pin(async move {
			engine::asset::Library::scan_pak_directory().await?;
			Ok(true)
		})
	}

	fn create_display(
		&mut self,
		_engine: &Arc<RwLock<Engine>>,
		event_loop: &EventLoop<()>,
	) -> anyhow::Result<()> {
		let window = Window::builder()
			.with_title("Triangle Demo")
			.with_size(800.0, 600.0)
			.with_resizable(true)
			.with_application::<TriangleDemo>()
			.build(event_loop)?;
		let render_phase = {
			let mut chain = window.graphics_chain().write().unwrap();
			chain.apply_procedure::<DefaultProcedure>()?.into_inner()
		};
		let renderer = renderer::Triangle::new(window.graphics_chain(), &render_phase)?;
		self.window = Some(window);
		self.renderer = Some(renderer);
		Ok(())
	}

	fn get_display_chain(&self) -> Option<Arc<RwLock<Chain>>> {
		self.window
			.as_ref()
			.map(|window| window.graphics_chain().clone())
	}
}

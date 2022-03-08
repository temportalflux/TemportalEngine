use anyhow::Result;
use engine::{graphics::chain::procedure::DefaultProcedure, Application};
pub use temportal_engine as engine;

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

pub fn run() -> Result<()> {
	engine::logging::init(&engine::logging::default_path(TriangleDemo::name(), None))?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	engine::window::Window::builder()
		.with_title("Triangle Demo")
		.with_size(800.0, 600.0)
		.with_resizable(true)
		.with_application::<TriangleDemo>()
		.build(&mut engine)?;

	let render_phase = {
		let arc = engine.display_chain().unwrap();
		let mut chain = arc.write().unwrap();
		chain.apply_procedure::<DefaultProcedure>()?.into_inner()
	};

	let _renderer = renderer::Triangle::new(engine.display_chain().unwrap(), &render_phase);

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

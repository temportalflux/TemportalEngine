use engine::{utility::VoidResult, Application};
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

pub fn run() -> VoidResult {
	engine::logging::init(TriangleDemo::name(), None)?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	engine::window::Window::builder()
		.with_title("Triangle Demo")
		.with_size(800.0, 600.0)
		.with_resizable(true)
		.with_application::<TriangleDemo>()
		.build(&mut engine)?;

	let _renderer = renderer::Triangle::new(engine.render_chain().unwrap());

	let engine = engine.make_threadsafe();
	engine::Engine::run(engine.clone(), || {})
}

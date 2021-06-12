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
	engine::logging::init::<TriangleDemo>(None)?;
	let engine = engine::Engine::new()?;
	engine.scan_paks()?;

	let mut window = engine::window::Window::builder()
		.with_title("Triangle Demo")
		.with_size(800.0, 600.0)
		.with_resizable(true)
		.with_application::<TriangleDemo>()
		.build(&engine)?;

	let chain = window.create_render_chain(engine::graphics::renderpass::Info::default())?;
	let _renderer = renderer::Triangle::new(&chain);

	engine.run(chain);
	Ok(())
}

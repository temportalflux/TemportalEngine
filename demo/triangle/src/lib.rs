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
	fn display_name() -> &'static str {
		"Triangle Demo"
	}
	fn location() -> &'static str {
		std::env!("CARGO_MANIFEST_DIR")
	}
	fn version() -> u32 {
		engine::utility::make_version(
			std::env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
		)
	}
}

pub fn run() -> VoidResult {
	let engine = engine::Engine::new::<TriangleDemo>(None)?;

	let mut window = engine::window::Window::builder()
		.with_title(TriangleDemo::display_name())
		.with_size(800.0, 600.0)
		.with_resizable(true)
		.with_application::<TriangleDemo>()
		.build(&engine)?;

	let chain = window.create_render_chain(engine::graphics::renderpass::Info::default())?;
	let _renderer = renderer::Triangle::new(&chain);

	engine.run(chain);
	Ok(())
}

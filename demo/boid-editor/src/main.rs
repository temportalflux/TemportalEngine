use boid;
use engine::{utility::VoidResult, Application};
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> VoidResult {
	engine::logging::init(boid::BoidDemo::name(), Some("_editor"))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<boid::BoidDemo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}

	Ok(())
}

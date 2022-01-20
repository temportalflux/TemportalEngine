use boid;
use engine::{utility::Result, Application};
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> Result<()> {
	engine::logging::init(&engine::logging::default_path(
		boid::BoidDemo::name(),
		Some("_editor"),
	))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<boid::BoidDemo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}

	Ok(())
}

use boid;
use engine::utility::VoidResult;
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> VoidResult {
	let _ = engine::Engine::new::<boid::BoidDemo>(Some("_editor"))?;
	editor::Editor::initialize::<boid::BoidDemo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}

	Ok(())
}

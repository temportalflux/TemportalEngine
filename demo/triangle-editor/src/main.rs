use demo_triangle::TriangleDemo;
use engine::utility::VoidResult;
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> VoidResult {
	engine::logging::init::<TriangleDemo>(Some("_editor"))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<TriangleDemo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}
	Ok(())
}

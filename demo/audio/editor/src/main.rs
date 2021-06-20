use engine::{utility::VoidResult, Application};
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> VoidResult {
	engine::logging::init(demo_audio::Demo::name(), Some("_editor"))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<demo_audio::Demo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}
	Ok(())
}

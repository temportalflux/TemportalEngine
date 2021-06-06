use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> engine::utility::VoidResult {
	let _ = engine::Engine::new::<demo_ui::UIDemo>(Some("_editor"))?;
	editor::Editor::initialize::<demo_ui::UIDemo>()?;
	let _ = editor::Editor::read().run_commandlets()?;
	Ok(())
}

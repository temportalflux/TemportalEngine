use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> engine::utility::VoidResult {
	editor::Editor::initialize::<demo_ui::UIDemo>()?;
	let _ = editor::Editor::read().run_commandlets()?;
	Ok(())
}

use temportal_engine as engine;
use temportal_engine_editor as editor;
use engine::Application;

fn main() -> engine::utility::VoidResult {
	engine::logging::init(demo_ui::UIDemo::name(), Some("_editor"))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<demo_ui::UIDemo>()?;
	let _ = editor::Editor::read().run_commandlets()?;
	Ok(())
}

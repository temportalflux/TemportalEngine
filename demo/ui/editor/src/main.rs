use engine::Application;

fn main() -> anyhow::Result<()> {
	engine::logging::init(&engine::logging::default_path(
		demo_ui::UIDemo::name(),
		Some("_editor"),
	))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<demo_ui::UIDemo>()?;
	let _ = editor::Editor::read().run_commandlets()?;
	Ok(())
}

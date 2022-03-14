use engine::Application;

fn main() -> anyhow::Result<()> {
	engine::logging::init(&engine::logging::default_path(
		demo_ui::UIDemo::name(),
		Some("_editor"),
	))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<demo_ui::UIDemo>({
		let mut asset_manager = editor::asset::Manager::new();
		editor::audio::register_asset_types(&mut asset_manager);
		editor::graphics::register_asset_types(&mut asset_manager);
		asset_manager
	})?;
	let _ = editor::Editor::read().run_commandlets()?;
	Ok(())
}

use anyhow::Result;
use engine::Application;
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> Result<()> {
	engine::logging::init(&engine::logging::default_path(
		demo_audio::Demo::name(),
		Some("_editor"),
	))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<demo_audio::Demo>({
		let mut asset_manager = editor::asset::Manager::new();
		editor::audio::register_asset_types(&mut asset_manager);
		editor::graphics::register_asset_types(&mut asset_manager);
		asset_manager
	})?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}
	Ok(())
}

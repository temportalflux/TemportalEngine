use engine::{ Application};
use anyhow::Result;
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> Result<()> {
	engine::logging::init(&engine::logging::default_path(
		chat_room::ChatRoom::name(),
		Some("_editor"),
	))?;
	let _ = engine::Engine::new()?;
	editor::Editor::initialize::<chat_room::ChatRoom>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}
	Ok(())
}

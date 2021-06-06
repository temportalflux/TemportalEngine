use demo_triangle;
use engine::utility::VoidResult;
use temportal_engine as engine;
use temportal_engine_editor as editor;

fn main() -> VoidResult {
	editor::Editor::initialize::<demo_triangle::TriangleDemo>()?;
	if editor::Editor::read().run_commandlets()? {
		return Ok(());
	}
	Ok(())
}

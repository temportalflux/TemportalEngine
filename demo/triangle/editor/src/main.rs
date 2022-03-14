use demo_triangle::TriangleDemo;
use engine::Application;

struct Runtime;
impl engine::Runtime for Runtime {
	fn logging_path() -> std::path::PathBuf {
		engine::logging::default_path(TriangleDemo::name(), Some("_editor"))
	}

	fn initialize<'a>(
		&'a self,
		_engine: std::sync::Arc<std::sync::RwLock<engine::Engine>>,
	) -> engine::task::PinFutureResultLifetime<'a, bool> {
		Box::pin(async move {
			self.create_editor().await?;
			editor::Editor::run_commandlets().await;
			Ok(false)
		})
	}
}

impl Runtime {
	async fn create_editor(&self) -> anyhow::Result<()> {
		let editor = editor::Editor::new(self.create_asset_manager()).await?;
		editor::Editor::initialize(editor)
	}

	fn create_asset_manager(&self) -> editor::asset::Manager {
		let mut manager = editor::asset::Manager::new();
		editor::register_asset_types(&mut manager);
		manager
	}
}

fn main() {
	engine::run(Runtime)
}

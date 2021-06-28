use engine::{utility::VoidResult, Application};
pub use temportal_engine as engine;

#[path = "ui/mod.rs"]
pub mod ui;

pub struct ChatRoom();
impl Application for ChatRoom {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn run() -> VoidResult {
	engine::logging::init(ChatRoom::name(), None)?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	engine::window::Window::builder()
		.with_title("Chat Room")
		.with_size(1280.0, 720.0)
		.with_resizable(true)
		.with_application::<ChatRoom>()
		.with_clear_color([0.08, 0.08, 0.08, 1.0].into())
		.build(&mut engine)?;

	engine::ui::System::new(engine.render_chain().unwrap())?
		.with_engine_shaders()?
		.with_all_fonts()?
		.with_tree_root(engine::ui::make_widget!(ui::root::widget))
		.attach_system(&mut engine, None)?;

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

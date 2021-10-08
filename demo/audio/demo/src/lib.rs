use engine::{utility::VoidResult, Application};
pub use temportal_engine as engine;

#[path = "ui/mod.rs"]
mod ui;

pub struct Demo();
impl Application for Demo {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn run() -> VoidResult {
	engine::logging::init(&engine::logging::default_path(Demo::name(), None))?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	engine::window::Window::builder()
		.with_title("Audio Demo")
		.with_size(1280.0, 720.0)
		.with_resizable(false)
		.with_application::<Demo>()
		.build(&mut engine)?;

	{
		use ui::*;
		ui::System::new(engine.render_chain().unwrap())?
			.with_engine_shaders()?
			.with_all_fonts()?
			.with_texture(&Demo::get_asset_id("ui/cycle"))?
			.with_texture(&Demo::get_asset_id("ui/fast-forward"))?
			.with_texture(&Demo::get_asset_id("ui/pause"))?
			.with_texture(&Demo::get_asset_id("ui/play"))?
			.with_tree(raui::WidgetNode::Component(raui::make_widget!(
				ui::root::widget
			)))
			.attach_system(&mut engine, None)?;
	}

	crate::engine::audio::System::add_source(crate::Demo::get_asset_id("audio/music-for-manatees"));

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

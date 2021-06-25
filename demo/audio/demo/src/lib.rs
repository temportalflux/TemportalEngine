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
	engine::logging::init(Demo::name(), None)?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	let mut window = engine::window::Window::builder()
		.with_title("Audio Demo")
		.with_size(1280.0, 720.0)
		.with_resizable(false)
		.with_application::<Demo>()
		.build(&engine)?;

	let chain = window.create_render_chain(engine::graphics::renderpass::Info::default())?;

	{
		use ui::*;
		ui::System::new(&chain)?
			.with_engine_shaders()?
			.with_all_fonts()?
			.with_texture(&Demo::get_asset_id("ui/cycle"))?
			.with_texture(&Demo::get_asset_id("ui/fast-forward"))?
			.with_texture(&Demo::get_asset_id("ui/pause"))?
			.with_texture(&Demo::get_asset_id("ui/play"))?
			.with_tree(WidgetNode::Component(make_widget!(ui::root::widget)))
			.attach_system(&mut engine, &chain, None)?;
	}

	{
		use engine::audio;
		let mut audio_system = audio::System::write()?;
		let source = audio_system.create_sound(&Demo::get_asset_id("audio/click"))?;
		audio_system.add_persistent_source(source);
	}

	engine.run(chain.clone());
	Ok(())
}

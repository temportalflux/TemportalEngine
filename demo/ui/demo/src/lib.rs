use engine::{asset::statics, ui, utility::VoidResult, Application};
pub use temportal_engine as engine;

pub struct UIDemo();
impl Application for UIDemo {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn run() -> VoidResult {
	engine::logging::init(UIDemo::name(), None)?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	engine::window::Window::builder()
		.with_title("(RA)UI Demo")
		.with_size(1280.0, 720.0)
		.with_resizable(false)
		.with_application::<UIDemo>()
		.build(&mut engine)?;

	// Create the UI system and widget tree
	{
		use ui::*;
		ui::System::new(engine.render_chain().unwrap())?
			.with_engine_shaders()?
			.with_all_fonts()?
			.with_texture(&UIDemo::get_asset_id("textures/background"))?
			.with_tree(WidgetNode::Component(
				make_widget!(horizontal_box)
					.listed_slot(
						make_widget!(vertical_box)
							.listed_slot(make_widget!(text_box).key("hello_world").with_props(
								TextBoxProps {
									text: "Hello World!".to_owned(),
									color: utils::Color {
										r: 0.0,
										g: 1.0,
										b: 1.0,
										a: 1.0,
									},
									font: TextBoxFont {
										name: statics::font::unispace::REGULAR.to_owned(),
										size: 100.0,
									},
									..Default::default()
								},
							))
							.listed_slot(make_widget!(text_box).key("item2").with_props(
								TextBoxProps {
									text: "item 2\nand another line".to_owned(),
									color: utils::Color {
										r: 1.0,
										g: 0.0,
										b: 0.0,
										a: 1.0,
									},
									font: TextBoxFont {
										name: statics::font::unispace::REGULAR.to_owned(),
										size: 20.0,
									},
									..Default::default()
								},
							))
							.listed_slot(make_widget!(text_box).key("item3").with_props(
								TextBoxProps {
									text: "fdsa".to_owned(),
									color: utils::Color {
										r: 1.0,
										g: 1.0,
										b: 1.0,
										a: 1.0,
									},
									font: TextBoxFont {
										name: statics::font::unispace::REGULAR.to_owned(),
										size: 20.0,
									},
									..Default::default()
								},
							)),
					)
					.listed_slot(
						make_widget!(vertical_box)
							.listed_slot(make_widget!(text_box).key("row1column2").with_props(
								TextBoxProps {
									text: "C".to_owned(),
									color: utils::Color {
										r: 0.0,
										g: 0.0,
										b: 1.0,
										a: 1.0,
									},
									font: TextBoxFont {
										name: statics::font::unispace::REGULAR.to_owned(),
										size: 30.0,
									},
									..Default::default()
								},
							))
							.listed_slot(make_widget!(image_box).key("row2column2").with_props(
								ImageBoxProps {
									material: ImageBoxMaterial::Color(ImageBoxColor {
										color: Color {
											r: 1.0,
											g: 0.0,
											b: 1.0,
											a: 1.0,
										},
										..Default::default()
									}),
									..Default::default()
								},
							))
							.listed_slot(make_widget!(image_box).key("row3column2").with_props(
								ImageBoxProps {
									material: ImageBoxMaterial::Image(ImageBoxImage {
										id: "textures/background".to_owned(),
										scaling: ImageBoxImageScaling::Frame(ImageBoxFrame {
											source: Rect {
												top: 5.0,
												right: 5.0,
												bottom: 5.0,
												left: 5.0,
											},
											destination: Rect {
												top: 20.0,
												right: 20.0,
												bottom: 20.0,
												left: 20.0,
											},
											frame_only: false,
											frame_keep_aspect_ratio: false,
										}),
										..Default::default()
									}),
									..Default::default()
								},
							)),
					),
			))
			.attach_system(&mut engine, None)?;
	}

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

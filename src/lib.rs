extern crate sdl2;
extern crate shaderc;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;
use std::{cell::RefCell, rc::Rc};

use structopt::StructOpt;
use temportal_graphics::{
	self, command,
	flags::{self, ColorComponent, Format},
	pipeline, renderpass, shader, AppInfo, Context,
};
use temportal_math::Vector;

#[path = "asset/lib.rs"]
pub mod asset;

#[path = "build/lib.rs"]
pub mod build;

#[path = "display/lib.rs"]
pub mod display;

#[path = "graphics/lib.rs"]
pub mod graphics;

#[path = "world/lib.rs"]
pub mod world;

#[path = "utility/lib.rs"]
pub mod utility;

#[derive(Debug, StructOpt)]
struct Opt {
	/// Use validation layers
	#[structopt(short, long)]
	validation_layers: bool,
	#[structopt(short, long)]
	build: bool,
}

pub struct Engine {
	run_build_commandlet: bool,
	pub build_assets_callback: Option<build::BuildAssetsCallback>,

	vulkan_validation_enabled: bool,
	graphics_context: Context,
	app_info: AppInfo,

	pub assets: EngineAssets,

	quit_has_been_triggered: bool,
}

pub struct EngineAssets {
	pub types: asset::TypeRegistry,
	pub library: asset::Library,
	pub loader: asset::Loader,
}

impl Engine {
	pub fn new() -> Result<Engine, Box<dyn std::error::Error>> {
		use asset::Asset;

		let flags = Opt::from_args();
		let graphics_context = Context::new()?;
		let app_info = AppInfo::new(&graphics_context)
			.engine("TemportalEngine", utility::make_version(0, 1, 0));
		let mut engine = Engine {
			run_build_commandlet: flags.build,
			build_assets_callback: None,

			vulkan_validation_enabled: flags.validation_layers,
			graphics_context,
			app_info,

			assets: EngineAssets {
				types: asset::TypeRegistry::new(),
				library: asset::Library::new(),
				loader: asset::Loader::new(),
			},

			quit_has_been_triggered: false,
		};

		engine.assets.types.register(graphics::Shader::type_data());

		Ok(engine)
	}

	pub fn set_application(mut self, name: &str, version: u32) -> Self {
		self.app_info.set_application_info(name, version);
		self
	}

	pub fn app_info(&self) -> &AppInfo {
		&self.app_info
	}

	pub fn create_display_manager(engine: &Rc<RefCell<Self>>) -> utility::Result<display::Manager> {
		let mut manager = display::Manager::new(engine.clone())?;
		let weak_engine = Rc::downgrade(engine);
		manager.add_event_listener(weak_engine);
		Ok(manager)
	}

	pub fn is_build_instance(&self) -> bool {
		self.run_build_commandlet
	}

	pub fn build(&self) -> Result<(), Box<dyn std::error::Error>> {
		match self.build_assets_callback {
			Some(callback) => return build::run(callback),
			None => panic!("No valid assets callback provided"),
		}
	}
}

impl display::EventListener for Engine {
	fn on_event(&mut self, event: &sdl2::event::Event) {
		match event {
			Event::Quit { .. } => self.quit_has_been_triggered = true,
			Event::KeyDown {
				keycode: Some(Keycode::Escape),
				..
			} => self.quit_has_been_triggered = true,
			_ => {}
		}
	}
}

pub fn run(
	engine: &Rc<RefCell<Engine>>,
	display: &mut display::Manager,
	window: &mut Rc<RefCell<display::Window>>,
	vert_shader: Vec<u8>,
	frag_shader: Vec<u8>,
) -> Result<(), Box<dyn std::error::Error>> {
	let vert_shader = shader::Module::create(
		window.borrow().logical().clone(),
		shader::Info {
			kind: flags::ShaderStageKind::VERTEX,
			entry_point: String::from("main"),
			bytes: vert_shader,
		},
	)?;
	let frag_shader = shader::Module::create(
		window.borrow().logical().clone(),
		shader::Info {
			kind: flags::ShaderStageKind::FRAGMENT,
			entry_point: String::from("main"),
			bytes: frag_shader,
		},
	)?;

	let render_pass = {
		let mut rp_info = renderpass::Info::default();

		let frame_attachment_index = rp_info.attach(
			renderpass::Attachment::default()
				.set_format(Format::B8G8R8A8_SRGB)
				.set_sample_count(flags::SampleCount::_1)
				.set_general_ops(renderpass::AttachmentOps {
					load: flags::AttachmentLoadOp::CLEAR,
					store: flags::AttachmentStoreOp::STORE,
				})
				.set_final_layout(flags::ImageLayout::PRESENT_SRC_KHR),
		);

		let main_pass_index =
			rp_info.add_subpass(renderpass::Subpass::default().add_attachment_ref(
				frame_attachment_index,
				flags::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
			));

		rp_info.add_dependency(
			renderpass::Dependency::new(None)
				.set_stage(flags::PipelineStage::COLOR_ATTACHMENT_OUTPUT),
			renderpass::Dependency::new(Some(main_pass_index))
				.set_stage(flags::PipelineStage::COLOR_ATTACHMENT_OUTPUT)
				.set_access(flags::Access::COLOR_ATTACHMENT_WRITE),
		);

		rp_info.create_object(window.borrow().logical().clone())?
	};

	let pipeline_layout = pipeline::Layout::create(window.borrow().logical().clone())?;
	let pipeline = pipeline::Info::default()
		.add_shader(&vert_shader)
		.add_shader(&frag_shader)
		.set_viewport_state(
			pipeline::ViewportState::default()
				.add_viewport(
					temportal_graphics::utility::Viewport::default()
						.set_size(window.borrow().physical().image_extent()),
				)
				.add_scissor(
					temportal_graphics::utility::Scissor::default()
						.set_size(window.borrow().physical().image_extent()),
				),
		)
		.set_rasterization_state(pipeline::RasterizationState::default())
		.set_color_blending(pipeline::ColorBlendState::default().add_attachment(
			pipeline::ColorBlendAttachment {
				color_flags: ColorComponent::R
					| ColorComponent::G | ColorComponent::B
					| ColorComponent::A,
			},
		))
		.create_object(
			window.borrow().logical().clone(),
			&pipeline_layout,
			&render_pass,
		)?;

	let mut framebuffers: Vec<command::framebuffer::Framebuffer> = Vec::new();
	for image_view in window.borrow().frame_views().iter() {
		framebuffers.push(
			command::framebuffer::Info::default()
				.set_extent(window.borrow().physical().image_extent())
				.create_object(&image_view, &render_pass, window.borrow().logical().clone())?,
		);
	}

	// END: Initialization

	// START: Recording Cmd Buffers

	let record_instruction = renderpass::RecordInstruction::default()
		.set_extent(window.borrow().physical().image_extent())
		.clear_with(renderpass::ClearValue::Color(Vector::new([
			0.0, 0.0, 0.0, 1.0,
		])));
	for (cmd_buffer, frame_buffer) in window
		.borrow()
		.command_buffers()
		.iter()
		.zip(framebuffers.iter())
	{
		cmd_buffer.begin()?;
		cmd_buffer.start_render_pass(&frame_buffer, &render_pass, record_instruction.clone());
		cmd_buffer.bind_pipeline(&pipeline, flags::PipelineBindPoint::GRAPHICS);
		//cmd_buffer.draw(3, 0, 1, 0, 0);
		window.borrow().logical().draw(&cmd_buffer, 3);
		cmd_buffer.stop_render_pass();
		cmd_buffer.end()?;
	}

	// END: Recording Cmd Buffers

	// Game loop
	while !engine.borrow().quit_has_been_triggered {
		display.poll_all_events()?;
		window.borrow_mut().render_frame()?;
		::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
	}

	window.borrow().logical().wait_until_idle()?;

	Ok(())
}

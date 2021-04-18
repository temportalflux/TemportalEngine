extern crate sdl2;
extern crate shaderc;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::rc::Rc;
use std::time::Duration;

use structopt::StructOpt;
use temportal_graphics::{
	self, command,
	device::{logical, swapchain},
	flags::{
		self, ColorComponent, ColorSpace, CompositeAlpha, Format, ImageAspect, ImageUsageFlags,
		ImageViewType, SharingMode,
	},
	image, pipeline, renderpass, shader,
	structs::ImageSubresourceRange,
	AppInfo, Context,
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
}

pub struct EngineAssets {
	pub types: asset::TypeRegistry,
	pub library: asset::Library,
	pub loader: asset::Loader,
}

impl Engine {
	pub fn set_application(mut self, name: &str, version: u32) -> Self {
		self.app_info.set_application_info(name, version);
		self
	}

	pub fn app_info(&self) -> &AppInfo {
		&self.app_info
	}

	pub fn create_display_manager(engine: &Rc<Self>) -> utility::Result<display::Manager> {
		display::Manager::new(engine.clone())
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

pub fn init() -> Result<Engine, Box<dyn std::error::Error>> {
	use asset::Asset;

	let flags = Opt::from_args();
	let graphics_context = Context::new()?;
	let app_info =
		AppInfo::new(&graphics_context).engine("TemportalEngine", utility::make_version(0, 1, 0));
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
	};

	engine.assets.types.register(graphics::Shader::type_data());

	Ok(engine)
}

pub fn run(
	display: &display::Manager,
	window: &display::Window,
	vert_shader: Vec<u8>,
	frag_shader: Vec<u8>,
) -> Result<(), Box<dyn std::error::Error>> {
	let permitted_frame_count = window.physical().image_count_range();
	let frame_count = std::cmp::min(
		std::cmp::max(3, permitted_frame_count.start),
		permitted_frame_count.end,
	);

	let swapchain = swapchain::Info::default()
		.set_image_count(frame_count)
		.set_image_format(Format::B8G8R8A8_SRGB)
		.set_image_color_space(ColorSpace::SRGB_NONLINEAR_KHR)
		.set_image_extent(window.physical().image_extent())
		.set_image_array_layer_count(1)
		.set_image_usage(ImageUsageFlags::COLOR_ATTACHMENT)
		.set_image_sharing_mode(SharingMode::EXCLUSIVE)
		.set_pre_transform(window.physical().current_transform())
		.set_composite_alpha(CompositeAlpha::OPAQUE_KHR)
		.set_present_mode(window.physical().selected_present_mode)
		.set_is_clipped(true)
		.create_object(window.logical().clone(), &window.surface())?;
	let frame_images = swapchain.get_images()?;

	let mut frame_image_views: Vec<image::View> = Vec::new();
	for image in frame_images.iter() {
		frame_image_views.push(
			image::ViewInfo::new()
				.set_view_type(ImageViewType::_2D)
				.set_format(Format::B8G8R8A8_SRGB)
				.set_subresource_range(ImageSubresourceRange {
					aspect_mask: ImageAspect::COLOR,
					base_mip_level: 0,
					level_count: 1,
					base_array_layer: 0,
					layer_count: 1,
				})
				.create_object(window.logical().clone(), &image)?,
		);
	}

	let vert_shader = shader::Module::create(
		window.logical().clone(),
		shader::Info {
			kind: flags::ShaderStageKind::VERTEX,
			entry_point: String::from("main"),
			bytes: vert_shader,
		},
	)?;
	let frag_shader = shader::Module::create(
		window.logical().clone(),
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

		rp_info.create_object(window.logical().clone())?
	};

	let pipeline_layout = pipeline::Layout::create(window.logical().clone())?;
	let pipeline = pipeline::Info::default()
		.add_shader(&vert_shader)
		.add_shader(&frag_shader)
		.set_viewport_state(
			pipeline::ViewportState::default()
				.add_viewport(
					temportal_graphics::utility::Viewport::default()
						.set_size(window.physical().image_extent()),
				)
				.add_scissor(
					temportal_graphics::utility::Scissor::default()
						.set_size(window.physical().image_extent()),
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
		.create_object(window.logical().clone(), &pipeline_layout, &render_pass)?;

	let mut framebuffers: Vec<command::framebuffer::Framebuffer> = Vec::new();
	for image_view in frame_image_views.iter() {
		framebuffers.push(
			command::framebuffer::Info::default()
				.set_extent(window.physical().image_extent())
				.create_object(&image_view, &render_pass, window.logical().clone())?,
		);
	}

	let cmd_pool = command::Pool::create(window.logical().clone(), window.graphics_queue_index())?;
	let cmd_buffers = cmd_pool.allocate_buffers(framebuffers.len())?;

	// END: Initialization

	// START: Recording Cmd Buffers

	let record_instruction = renderpass::RecordInstruction::default()
		.set_extent(window.physical().image_extent())
		.clear_with(renderpass::ClearValue::Color(Vector::new([
			0.0, 0.0, 0.0, 1.0,
		])));
	for (cmd_buffer, frame_buffer) in cmd_buffers.iter().zip(framebuffers.iter()) {
		cmd_buffer.begin()?;
		cmd_buffer.start_render_pass(&frame_buffer, &render_pass, record_instruction.clone());
		cmd_buffer.bind_pipeline(&pipeline, flags::PipelineBindPoint::GRAPHICS);
		//cmd_buffer.draw(3, 0, 1, 0, 0);
		window.logical().draw(&cmd_buffer, 3);
		cmd_buffer.stop_render_pass();
		cmd_buffer.end()?;
	}

	// END: Recording Cmd Buffers

	let frames_in_flight = 2;
	let graphics_queue =
		logical::Device::get_queue(window.logical().clone(), window.graphics_queue_index());
	let img_available_semaphores =
		logical::Device::create_semaphores(&window.logical(), frames_in_flight)?;
	let render_finished_semaphores =
		logical::Device::create_semaphores(&window.logical(), frames_in_flight)?;
	let in_flight_fences = logical::Device::create_fences(
		&window.logical(),
		frames_in_flight,
		flags::FenceState::SIGNALED,
	)?;
	let mut images_in_flight: Vec<Option<&command::Fence>> =
		frame_images.iter().map(|_| None).collect::<Vec<_>>();
	let mut frame = 0;

	// Game loop
	let mut event_pump = display.event_pump()?;
	'gameloop: loop {
		for event in event_pump.poll_iter() {
			match event {
				Event::Quit { .. } => break 'gameloop,
				Event::KeyDown {
					keycode: Some(Keycode::Escape),
					..
				} => break 'gameloop,
				_ => {}
			}
		}

		// START: Render

		// Wait for the previous frame/image to no longer be displayed
		window
			.logical()
			.wait_for(&in_flight_fences[frame], true, u64::MAX)?;
		// Get the index of the next image to display
		let next_image_idx =
			swapchain.acquire_next_image(u64::MAX, Some(&img_available_semaphores[frame]), None)?;
		// Ensure that the image for the next index is not being written to or displayed
		{
			let img_in_flight = &images_in_flight[next_image_idx];
			if img_in_flight.is_some() {
				window
					.logical()
					.wait_for(img_in_flight.unwrap(), true, u64::MAX)?;
			}
		}
		// Denote that the image that is in-flight is the fence for the this frame
		images_in_flight[next_image_idx] = Some(&in_flight_fences[frame]);

		// Mark the image as not having been signaled (it is now being used)
		window.logical().reset_fences(&[&in_flight_fences[frame]])?;

		graphics_queue.submit(
			vec![command::SubmitInfo::default()
				// tell the gpu to wait until the image is available
				.wait_for(
					&img_available_semaphores[frame],
					flags::PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				)
				// denote which command buffer is being executed
				.add_buffer(&cmd_buffers[next_image_idx])
				// tell the gpu to signal a semaphore when the image is available again
				.signal_when_complete(&render_finished_semaphores[frame])],
			Some(&in_flight_fences[frame]),
		)?;

		graphics_queue.present(
			command::PresentInfo::default()
				.wait_for(&render_finished_semaphores[frame])
				.add_swapchain(&swapchain)
				.add_image_index(next_image_idx as u32),
		)?;

		frame = (frame + 1) % frames_in_flight;
		// END: RENDER

		::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
	}

	window.logical().wait_until_idle()?;

	Ok(())
}

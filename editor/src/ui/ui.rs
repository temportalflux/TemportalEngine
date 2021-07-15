use crate::{
	engine::{
		self,
		graphics::{
			command, descriptor, flags, pipeline, structs,
			types::{Vec2, Vec4},
			utility::Scissor,
			vertex_object, DescriptorCache, Drawable, ImageCache, Mesh, RenderChain,
			RenderChainElement,
		},
		math::nalgebra::{Vector2, Vector4},
		utility::AnyError,
		Application, Engine, WinitEventListener,
	},
	ui, Editor,
};
use imgui_winit_support::{HiDpiMode, WinitPlatform};
use std::{
	sync::{Arc, Mutex, RwLock, Weak},
	time::Instant,
};

pub struct Ui {
	pending_gpu_signals: Vec<Arc<command::Semaphore>>,
	frames: Vec<Frame>,
	descriptor_cache: DescriptorCache<imgui::TextureId>,
	image_cache: ImageCache<imgui::TextureId>,
	drawable: Drawable,
	ui_elements: Vec<Weak<RwLock<dyn ui::Element>>>,
	last_frame: Instant,
	imgui_ctx: Arc<Mutex<imgui::Context>>,
	imgui_winit: RwLock<WinitPlatform>,
	window_handle: Weak<RwLock<winit::window::Window>>,
}

struct Frame {
	draw_calls: Vec<DrawCall>,
	mesh: Mesh<u32, Vertex>,
}

struct DrawCall {
	texture_id: imgui::TextureId,
	scissor: Scissor,
	index_count: usize,
	first_index: usize,
	vertex_offset: usize,
}

#[vertex_object]
#[derive(Debug, Default)]
pub struct Vertex {
	#[vertex_attribute([R, G], Bit32, SFloat)]
	pos: Vec2,

	#[vertex_attribute([R, G], Bit32, SFloat)]
	tex_coord: Vec2,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	color: Vec4,
}

impl Vertex {
	pub fn with_position(mut self, pos: Vector2<f32>) -> Self {
		self.pos = pos.into();
		self
	}

	pub fn with_tex_coord(mut self, coord: Vector2<f32>) -> Self {
		self.tex_coord = coord.into();
		self
	}

	pub fn with_color(mut self, color: Vector4<f32>) -> Self {
		self.color = color.into();
		self
	}
}

impl Ui {
	pub fn create(engine: &mut Engine) -> Result<Arc<RwLock<Self>>, AnyError> {
		let strong = Arc::new(RwLock::new(Self::new(engine)?));
		if let Some(mut chain) = engine.render_chain_write() {
			chain.add_render_chain_element(None, &strong)?;
		}
		engine.add_winit_listener(&strong);
		Ok(strong)
	}

	fn new(engine: &mut Engine) -> engine::utility::Result<Ui> {
		let window_handle = engine.window().unwrap().unwrap();

		let mut imgui_ctx = imgui::Context::create();
		imgui_ctx.set_ini_filename(None);
		imgui_ctx.style_mut().use_dark_colors();

		let mut imgui_winit = WinitPlatform::init(&mut imgui_ctx);
		if let Some(handle) = window_handle.upgrade() {
			if let Ok(window) = handle.read() {
				imgui_winit.attach_window(imgui_ctx.io_mut(), &window, HiDpiMode::Default);
			}
		}

		Ok(Ui {
			window_handle,
			imgui_ctx: Arc::new(Mutex::new(imgui_ctx)),
			imgui_winit: RwLock::new(imgui_winit),
			last_frame: Instant::now(),
			ui_elements: Vec::new(),
			drawable: Drawable::default(),
			image_cache: ImageCache::default().with_cache_name("EditorUI.Image"),
			descriptor_cache: DescriptorCache::new(
				descriptor::layout::SetLayout::builder()
					.with_binding(
						0,
						flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
						1,
						flags::ShaderKind::Fragment,
					)
					.build(&engine.render_chain().unwrap().read().unwrap().logical())?,
			),
			frames: Vec::new(),
			pending_gpu_signals: Vec::new(),
		})
	}

	pub fn add_element<T>(&mut self, element: &Arc<RwLock<T>>)
	where
		T: ui::Element + 'static,
	{
		let element_strong: Arc<RwLock<dyn ui::Element>> = element.clone();
		self.ui_elements.push(Arc::downgrade(&element_strong));
	}
}

impl WinitEventListener for Ui {
	fn on_event(&mut self, event: &winit::event::Event<()>) {
		use winit::event::Event;
		let mut ctx = self.imgui_ctx.lock().unwrap();
		match event {
			Event::NewEvents(_) => {
				let now = Instant::now();
				ctx.io_mut().update_delta_time(now - self.last_frame);
				self.last_frame = now;
			}
			event => {
				if let Ok(mut platform) = self.imgui_winit.write() {
					if let Some(handle) = self.window_handle.upgrade() {
						if let Ok(window) = handle.read() {
							platform.handle_event(ctx.io_mut(), &window, event);
						}
					}
				}
			}
		}
	}
}

// This isnt good practice, but is necessary because `imgui::Context` contains a raw mutable pointer.
// It is "safe" to tell rust that Ui is Send because the context is an Arc<Mutex<..>>
unsafe impl Send for Ui {}
unsafe impl Sync for Ui {}

impl RenderChainElement for Ui {
	fn name(&self) -> &'static str {
		"imgui"
	}

	fn initialize_with(
		&mut self,
		chain: &mut RenderChain,
	) -> Result<Vec<Arc<command::Semaphore>>, AnyError> {
		self.drawable
			.add_shader(&Editor::get_asset_id("imgui_vert"))?;
		self.drawable
			.add_shader(&Editor::get_asset_id("imgui_frag"))?;
		self.drawable.create_shaders(chain)?;

		for i in 0..chain.frame_count() {
			self.frames.push(Frame {
				mesh: Mesh::<u32, Vertex>::new(
					format!("EditorUI.Frame{}.Mesh", i),
					&chain.allocator(),
					10,
				)?,
				draw_calls: Vec::new(),
			});
		}

		{
			let mut ctx = self.imgui_ctx.lock().unwrap();
			let font: &mut imgui::FontAtlas = &mut ctx.fonts();
			let texture_id = font.tex_id;
			let tex = font.build_rgba32_texture();
			// TODO: font image should use a specific sampler
			// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp#L695
			self.image_cache.insert_compiled(
				texture_id.clone(),
				Some(format!("Font.{}", texture_id.id())),
				[tex.width as usize, tex.height as usize].into(),
				tex.data.to_vec(),
			);
		}

		Ok(self.take_gpu_signals())
	}

	fn destroy_render_chain(&mut self, chain: &RenderChain) -> Result<(), AnyError> {
		self.drawable.destroy_pipeline(chain)?;
		Ok(())
	}

	fn on_render_chain_constructed(
		&mut self,
		chain: &RenderChain,
		resolution: &Vector2<f32>,
		subpass_id: &Option<String>,
	) -> Result<(), AnyError> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		use pipeline::state::*;
		self.drawable.create_pipeline(
			chain,
			vec![self.descriptor_cache.layout()],
			pipeline::Pipeline::builder()
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::from(structs::Extent2D {
					width: resolution.x as u32,
					height: resolution.y as u32,
				}))
				.set_rasterization_state(
					Rasterization::default()
						.set_cull_mode(flags::CullMode::NONE)
						.set_front_face(flags::FrontFace::COUNTER_CLOCKWISE),
				)
				.set_color_blending(color_blend::ColorBlend::default().add_attachment(
					color_blend::Attachment {
						color_flags: flags::ColorComponent::R
							| flags::ColorComponent::G | flags::ColorComponent::B
							| flags::ColorComponent::A,
						blend: Some(color_blend::Blend {
							color: SrcAlpha * New + (One - SrcAlpha) * Old,
							alpha: One * New + (One - SrcAlpha) * Old,
						}),
					},
				))
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			subpass_id,
		)?;
		Ok(())
	}

	fn preframe_update(&mut self, chain: &RenderChain) -> Result<(), AnyError> {
		let (image_ids, mut signals) = self.image_cache.load_pending(chain)?;
		self.pending_gpu_signals.append(&mut signals);

		for image_id in image_ids.into_iter() {
			use descriptor::update::*;
			let cached_image = &self.image_cache[&image_id];
			let descriptor_set = self.descriptor_cache.insert(image_id, chain)?;
			Queue::default()
				.with(Operation::Write(WriteOp {
					destination: Descriptor {
						set: descriptor_set.clone(),
						binding_index: 0,
						array_element: 0,
					},
					kind: flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					object: ObjectKind::Image(vec![ImageKind {
						sampler: cached_image.sampler.clone(),
						view: cached_image.view.clone(),
						layout: flags::ImageLayout::ShaderReadOnlyOptimal,
					}]),
				}))
				.apply(&chain.logical());
		}

		let mut ctx = self.imgui_ctx.lock().unwrap();
		if let Ok(platform) = self.imgui_winit.read() {
			if let Some(handle) = self.window_handle.upgrade() {
				if let Ok(window) = handle.read() {
					platform.prepare_frame(ctx.io_mut(), &window)?;
				}
			}
		}
		Ok(())
	}

	fn prerecord_update(
		&mut self,
		chain: &RenderChain,
		_buffer: &command::Buffer,
		frame: usize,
		resolution: &Vector2<f32>,
	) -> Result<bool, AnyError> {
		self.ui_elements
			.retain(|element| element.strong_count() > 0);

		// Based on
		// - https://github.com/michaelfairley/rust-imgui-opengl-renderer/blob/master/src/lib.rs#L209
		// - https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp#L396
		let (buffer_data, draw_calls) = self.build_ui(resolution);
		if let Some(buffer_data) = buffer_data {
			let mut mesh_gpu_signals =
				self.frames[frame]
					.mesh
					.write(&buffer_data.0, &buffer_data.1, chain)?;
			self.pending_gpu_signals.append(&mut mesh_gpu_signals);
		}
		self.frames[frame].draw_calls = draw_calls;

		Ok(true)
	}

	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> Result<(), AnyError> {
		let frame = &self.frames[frame];

		self.drawable.bind_pipeline(buffer);
		frame.mesh.bind_buffers(buffer);
		for draw_call in frame.draw_calls.iter() {
			buffer.set_dynamic_scissors(vec![draw_call.scissor]);
			let descriptor_set = &self.descriptor_cache[&draw_call.texture_id];
			if let Some(descriptor_set) = descriptor_set.upgrade() {
				self.drawable
					.bind_descriptors(buffer, vec![&descriptor_set]);
				buffer.draw(
					draw_call.index_count,
					draw_call.first_index,
					1,
					0,
					draw_call.vertex_offset,
				);
			}
		}

		Ok(())
	}

	fn take_gpu_signals(&mut self) -> Vec<Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}
}

impl Ui {
	fn build_ui(
		&self,
		resolution: &Vector2<f32>,
	) -> (Option<(Vec<Vertex>, Vec<u32>)>, Vec<DrawCall>) {
		let mut ctx = self.imgui_ctx.lock().unwrap();

		let ui_builder = ctx.frame();

		for element in self.ui_elements.iter() {
			if let Some(widget) = element.upgrade() {
				if let Ok(mut locked) = widget.write() {
					locked.render(&ui_builder);
				}
			}
		}

		if let Ok(mut platform) = self.imgui_winit.write() {
			if let Some(handle) = self.window_handle.upgrade() {
				if let Ok(window) = handle.read() {
					platform.prepare_render(&ui_builder, &window);
				}
			}
		}

		let draw_data = ui_builder.render();
		let offset: Vector2<f32> = [-1.0, -1.0].into();

		// Transform draw data into vertex and index buffer data (to upload later)
		let mut buffer_data: Option<(Vec<Vertex>, Vec<u32>)> = None;
		if draw_data.total_vtx_count > 0 {
			let mut vertices = Vec::with_capacity(draw_data.total_vtx_count as usize);
			let mut indices = Vec::with_capacity(draw_data.total_idx_count as usize);
			for draw_list in draw_data.draw_lists() {
				for im_vert in draw_list.vtx_buffer().iter() {
					let pos: Vector2<f32> = im_vert.pos.into();
					let pos = pos.component_div(resolution);
					let pos = pos * 2.0 + offset;
					// color can be converted directly with array::map when its stable
					// https://github.com/rust-lang/rust/issues/75243
					// imgui sRGB and linear color spaces are conflated
					// https://github.com/ocornut/imgui/issues/578#issuecomment-379537376
					let color: Vector4<f32> = [
						(im_vert.col[0] as f32 / 255.0).powf(2.2),
						(im_vert.col[1] as f32 / 255.0).powf(2.2),
						(im_vert.col[2] as f32 / 255.0).powf(2.2),
						(im_vert.col[3] as f32 / 255.0).powf(2.2),
					]
					.into();
					let vertex = Vertex::default()
						.with_position(pos)
						.with_tex_coord(im_vert.uv.into())
						.with_color(color);
					vertices.push(vertex);
				}
				for im_index in draw_list.idx_buffer().iter() {
					// convert u16 indices to u32
					indices.push(*im_index as u32);
				}
			}
			buffer_data = Some((vertices, indices));
		}

		let mut draw_calls = Vec::new();
		let mut encountered_vertex_offset = 0;
		let mut encountered_index_offset = 0;
		for draw_list in draw_data.draw_lists() {
			for draw_cmd in draw_list.commands() {
				match draw_cmd {
					imgui::DrawCmd::Elements {
						count,
						cmd_params:
							imgui::DrawCmdParams {
								clip_rect: [x_min, y_min, x_max, y_max],
								texture_id,
								idx_offset,
								vtx_offset,
							},
						..
					} => {
						// eventually will need to support the display size and scale in imgui
						// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp#L488
						// https://github.com/michaelfairley/rust-imgui-opengl-renderer/blob/master/src/lib.rs#L180
						// https://github.com/michaelfairley/rust-imgui-opengl-renderer/blob/master/src/lib.rs#L234
						let scissor = Scissor::new(
							structs::Offset2D {
								x: x_min as i32,
								y: y_min as i32,
							},
							structs::Extent2D {
								width: (x_max - x_min) as u32,
								height: (y_max - y_min) as u32,
							},
						);
						draw_calls.push(DrawCall {
							texture_id,
							scissor,
							index_count: count,
							first_index: idx_offset + encountered_index_offset,
							vertex_offset: vtx_offset + encountered_vertex_offset,
						});
					}
					imgui::DrawCmd::ResetRenderState => {
						// NO-OP: not supported
						// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp#L481
						// https://github.com/michaelfairley/rust-imgui-opengl-renderer/blob/master/src/lib.rs#L244
						unimplemented!(
							"imgui::DrawCmd::ResetRenderState is not currently supported"
						);
					}
					imgui::DrawCmd::RawCallback { .. } => {
						// NO-OP: not supported
						// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp#L483
						// https://github.com/michaelfairley/rust-imgui-opengl-renderer/blob/master/src/lib.rs#L247
						unimplemented!("imgui::DrawCmd::RawCallback is not currently supported");
					}
				}
			}
			encountered_vertex_offset += draw_list.vtx_buffer().len();
			encountered_index_offset += draw_list.idx_buffer().len();
		}

		(buffer_data, draw_calls)
	}
}

use super::Element;
use crate::{
	graphics::{
		self, command, descriptor, flags, pipeline, structs,
		types::{Vec2, Vec4},
		utility::Scissor,
		utility::{BuildFromDevice, NameableBuilder},
		vertex_object, DescriptorCache, Drawable, ImageCache, Mesh, RenderChain,
		RenderChainElement,
	},
	math::nalgebra::{Vector2, Vector4},
	utility::AnyError,
	Engine, WinitEventListener,
};
use bytemuck::{Pod, Zeroable};
use copypasta::{ClipboardContext, ClipboardProvider};
use egui::{
	math::{pos2, vec2},
	CtxRef,
};
use std::{
	sync::{Arc, RwLock, Weak},
	time::Instant,
};
use winit::event::VirtualKeyCode;

type TextureId = egui::TextureId;

pub struct Ui {
	physical_size: winit::dpi::PhysicalSize<u32>,
	scale_factor: f64,

	context: CtxRef,
	raw_input: egui::RawInput,
	mouse_pos: egui::Pos2,
	modifiers_state: winit::event::ModifiersState,
	clipboard: ClipboardContext,
	current_cursor_icon: egui::CursorIcon,
	font_image_version: u64,

	pending_gpu_signals: Vec<Arc<command::Semaphore>>,
	frames: Vec<Frame>,
	descriptor_cache: DescriptorCache<TextureId>,
	image_cache: ImageCache<TextureId>,
	drawable: Drawable,
	render_callbacks: Vec<(Box<dyn Fn(&egui::CtxRef) -> bool>, /*retained*/ bool)>,
	last_frame: Instant,
	window_handle: Weak<RwLock<winit::window::Window>>,
}

struct Frame {
	draw_calls: Vec<DrawCall>,
	mesh: Mesh<u32, Vertex>,
}

struct DrawCall {
	texture_id: TextureId,
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
		Self::create_with_subpass(engine, None)
	}

	pub fn create_with_subpass(
		engine: &mut Engine,
		subpass_id: Option<String>,
	) -> Result<Arc<RwLock<Self>>, AnyError> {
		let strong = Arc::new(RwLock::new(Self::new(engine)?));
		engine
			.render_chain_write()
			.unwrap()
			.add_render_chain_element(subpass_id, &strong)?;
		engine.add_winit_listener(&strong);
		Ok(strong)
	}

	fn get_screen_rect(physical_size: &winit::dpi::PhysicalSize<u32>, scale_factor: f64) -> egui::Rect {
		egui::Rect::from_min_size(
			Default::default(),
			vec2(physical_size.width as f32, physical_size.height as f32) / scale_factor as f32,
		)
	}

	fn new(engine: &mut Engine) -> crate::utility::Result<Ui> {
		let window_handle = engine.window().unwrap().unwrap();

		let (physical_size, scale_factor) = match window_handle.upgrade() {
			Some(arc) => match arc.read() {
				Ok(guard) => (guard.inner_size(), guard.scale_factor()),
				_ => unimplemented!(),
			},
			_ => unimplemented!(),
		};

		// Create context
		let context = CtxRef::default();

		let raw_input = egui::RawInput {
			pixels_per_point: Some(scale_factor as f32),
			screen_rect: Some(Self::get_screen_rect(&physical_size, scale_factor)),
			time: Some(0.0),
			..Default::default()
		};

		// Create mouse pos and modifier state (These values are overwritten by handle events)
		let mouse_pos = pos2(0.0, 0.0);
		let modifiers_state = winit::event::ModifiersState::default();

		let clipboard = ClipboardContext::new().expect("Failed to initialize ClipboardContext.");

		Ok(Ui {
			physical_size,
			scale_factor,

			context,
			raw_input,
			mouse_pos,
			modifiers_state,
			clipboard,
			current_cursor_icon: egui::CursorIcon::None,
			font_image_version: 0,

			window_handle,
			last_frame: Instant::now(),
			render_callbacks: Vec::new(),
			drawable: Drawable::default().with_name("EditorUI.Gui"),
			image_cache: ImageCache::default().with_cache_name("EditorUI.Image"),
			descriptor_cache: DescriptorCache::new(
				descriptor::layout::SetLayout::builder()
					.with_name("EditorUI.DescriptorLayout")
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

	fn screen_size_scaled(&self) -> Vector2<f32> {
		let mut size: Vector2<f32> = [
			self.physical_size.width as f32,
			self.physical_size.height as f32,
		]
		.into();
		size /= self.scale_factor as f32;
		size
	}

	pub fn add_element<T>(&mut self, element: &Arc<RwLock<T>>)
	where
		T: Element + 'static,
	{
		let element_weak = Arc::downgrade(&element);
		self.add_render_callback(move |ctx| -> bool {
			match element_weak.upgrade() {
				Some(element) => {
					if let Ok(mut locked) = element.write() {
						locked.render(ctx);
					}
					true
				}
				None => false,
			}
		});
	}

	pub fn add_owned_element<T>(&mut self, element: T)
	where
		T: Element + 'static,
	{
		let arctex = std::sync::Arc::new(std::sync::Mutex::new(element));
		self.add_render_callback(move |ctx| -> bool {
			if let Ok(mut guard) = arctex.lock() {
				guard.render(ctx);
			}
			true
		});
	}

	/// Adds a callback to be executed each frame, which returns false when it should be removed from future rendering.
	pub fn add_render_callback<F>(&mut self, callback: F)
	where
		F: 'static + Fn(&egui::CtxRef) -> bool,
	{
		self.render_callbacks.push((Box::new(callback), true));
	}
}

impl WinitEventListener for Ui {
	fn on_event(&mut self, event: &winit::event::Event<()>) {
		use winit::event::{Event, WindowEvent};
		match event {
			Event::WindowEvent {
				window_id: _window_id,
				event,
			} => match event {
				WindowEvent::Resized(physical_size) => {
					let pixels_per_point = self
						.raw_input
						.pixels_per_point
						.unwrap_or_else(|| self.context.pixels_per_point());
					self.raw_input.screen_rect = Some(Self::get_screen_rect(&physical_size, pixels_per_point as f64));
				}
				// dpi changed
				WindowEvent::ScaleFactorChanged {
					scale_factor,
					new_inner_size,
				} => {
					self.scale_factor = *scale_factor;
					self.raw_input.pixels_per_point = Some(*scale_factor as f32);
					let pixels_per_point = self
						.raw_input
						.pixels_per_point
						.unwrap_or_else(|| self.context.pixels_per_point());
					self.raw_input.screen_rect = Some(Self::get_screen_rect(&new_inner_size, pixels_per_point as f64));
				}
				// mouse click
				WindowEvent::MouseInput { state, button, .. } => {
					if let Some(button) = super::conversions::mouse_button(*button) {
						self.raw_input.events.push(egui::Event::PointerButton {
							pos: self.mouse_pos,
							button,
							pressed: *state == winit::event::ElementState::Pressed,
							modifiers: super::conversions::modifiers(self.modifiers_state),
						});
					}
				}
				// mouse wheel
				WindowEvent::MouseWheel { delta, .. } => match delta {
					winit::event::MouseScrollDelta::LineDelta(x, y) => {
						let line_height = 24.0;
						self.raw_input.scroll_delta = vec2(*x, *y) * line_height;
					}
					winit::event::MouseScrollDelta::PixelDelta(delta) => {
						self.raw_input.scroll_delta = vec2(delta.x as f32, delta.y as f32);
					}
				},
				// mouse move
				WindowEvent::CursorMoved { position, .. } => {
					let pixels_per_point = self
						.raw_input
						.pixels_per_point
						.unwrap_or_else(|| self.context.pixels_per_point());
					let pos = pos2(
						position.x as f32 / pixels_per_point,
						position.y as f32 / pixels_per_point,
					);
					self.raw_input.events.push(egui::Event::PointerMoved(pos));
					self.mouse_pos = pos;
				}
				// mouse out
				WindowEvent::CursorLeft { .. } => {
					self.raw_input.events.push(egui::Event::PointerGone);
				}
				// modifier keys
				WindowEvent::ModifiersChanged(input) => self.modifiers_state = *input,
				// keyboard inputs
				WindowEvent::KeyboardInput { input, .. } => {
					if let Some(virtual_keycode) = input.virtual_keycode {
						let pressed = input.state == winit::event::ElementState::Pressed;
						if pressed {
							let is_ctrl = self.modifiers_state.ctrl();
							if is_ctrl && virtual_keycode == VirtualKeyCode::C {
								self.raw_input.events.push(egui::Event::Copy);
							} else if is_ctrl && virtual_keycode == VirtualKeyCode::X {
								self.raw_input.events.push(egui::Event::Cut);
							} else if is_ctrl && virtual_keycode == VirtualKeyCode::V {
								if let Ok(contents) = self.clipboard.get_contents() {
									self.raw_input.events.push(egui::Event::Text(contents));
								}
							} else if let Some(key) = super::conversions::key_code(virtual_keycode)
							{
								self.raw_input.events.push(egui::Event::Key {
									key,
									pressed: input.state == winit::event::ElementState::Pressed,
									modifiers: super::conversions::modifiers(self.modifiers_state),
								})
							}
						}
					}
				}
				// receive character
				WindowEvent::ReceivedCharacter(ch) => {
					// remove control character
					if ch.is_ascii_control() {
						return;
					}
					self.raw_input
						.events
						.push(egui::Event::Text(ch.to_string()));
				}
				_ => (),
			},
			_ => {}
		}
	}
}

unsafe impl Send for Ui {}
unsafe impl Sync for Ui {}

#[derive(Clone, Copy)]
#[repr(C)]
struct ScreenSizePushConstant {
	width: f32,
	height: f32,
}
unsafe impl Pod for ScreenSizePushConstant {}
unsafe impl Zeroable for ScreenSizePushConstant {}
impl From<Vector2<f32>> for ScreenSizePushConstant {
	fn from(other: Vector2<f32>) -> Self {
		Self {
			width: other.x,
			height: other.y,
		}
	}
}

impl RenderChainElement for Ui {
	fn name(&self) -> &'static str {
		"egui"
	}

	fn initialize_with(
		&mut self,
		chain: &mut RenderChain,
	) -> Result<Vec<Arc<command::Semaphore>>, AnyError> {
		use crate::{Application, EngineApp};
		let vertex = EngineApp::get_asset_id("shaders/debug/egui/vertex");
		let fragment = EngineApp::get_asset_id("shaders/debug/egui/fragment");

		self.drawable.add_shader(&vertex)?;
		self.drawable.add_shader(&fragment)?;
		self.drawable.create_shaders(chain)?;

		self.drawable.add_push_constant_range(
			pipeline::PushConstantRange::default()
				.with_stage(flags::ShaderKind::Vertex)
				.with_size(std::mem::size_of::<ScreenSizePushConstant>()),
		);

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
		use flags::blend::prelude::*;
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
						color_flags: R | G | B | A,
						blend: Some(color_blend::Blend {
							color: One * New + (One - SrcAlpha) * Old,
							alpha: One * New + Zero * Old,
						}),
					},
				))
				.with_depth_stencil(
					DepthStencil::default()
						.with_depth_test()
						.with_depth_write()
						.with_depth_compare_op(flags::CompareOp::LESS_OR_EQUAL)
						.with_stencil_front(
							StencilOpState::default()
								.with_fail_op(flags::StencilOp::KEEP)
								.with_pass_op(flags::StencilOp::KEEP)
								.with_compare_op(flags::CompareOp::ALWAYS),
						)
						.with_stencil_back(
							StencilOpState::default()
								.with_fail_op(flags::StencilOp::KEEP)
								.with_pass_op(flags::StencilOp::KEEP)
								.with_compare_op(flags::CompareOp::ALWAYS),
						),
				)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			subpass_id,
		)?;
		Ok(())
	}

	fn preframe_update(&mut self, chain: &RenderChain) -> Result<(), AnyError> {
		self.context.begin_frame(self.raw_input.take());

		{
			let texture = self.context.fonts().texture();
			if texture.version != self.font_image_version {
				use flags::format::prelude::*;
				self.image_cache.insert_pending(
					egui::TextureId::Egui,
					graphics::PendingEntry::new()
						.with_name("Font.egui".to_owned())
						.with_size([texture.width as usize, texture.height as usize].into())
						.with_format(flags::format::format(&[R, G, B, A], Bit8, UnsignedNorm))
						.with_binary(
							texture
								.pixels
								.iter()
								.flat_map(|&r| vec![r, r, r, r])
								.collect(),
						)
						.init_sampler(|sampler| {
							sampler.set_magnification(flags::Filter::LINEAR);
							sampler.set_minification(flags::Filter::LINEAR);
							sampler
								.set_address_modes([flags::SamplerAddressMode::CLAMP_TO_EDGE; 3]);
						}),
				);
				self.font_image_version = texture.version;
			}
		}

		let (image_ids, mut signals) = self.image_cache.load_pending(chain)?;
		self.pending_gpu_signals.append(&mut signals);

		for (image_id, image_name) in image_ids.into_iter() {
			use descriptor::update::*;
			let cached_image = &self.image_cache[&image_id];
			let descriptor_set = self.descriptor_cache.insert(
				image_id,
				image_name.map(|v| format!("EditorUI.{}", v)),
				chain,
			)?;
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

		Ok(())
	}

	fn prerecord_update(
		&mut self,
		chain: &RenderChain,
		_buffer: &command::Buffer,
		frame: usize,
		_resolution: &Vector2<f32>,
	) -> Result<bool, AnyError> {
		self.render_callbacks.retain(|(_, retained)| *retained);

		let (vertices, indices, draw_calls) = self.build_ui();
		let mesh = &mut self.frames[frame].mesh;
		let mut mesh_gpu_signals = mesh.write(&vertices, &indices, chain)?;
		self.pending_gpu_signals.append(&mut mesh_gpu_signals);
		self.frames[frame].draw_calls = draw_calls;

		// returns true to enforce immediate mode drawing (rerecording every frame)
		Ok(true)
	}

	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> Result<(), AnyError> {
		let frame = &self.frames[frame];

		buffer.begin_label("Draw:EditorUI", graphics::debug::LABEL_COLOR_DRAW);

		self.drawable.bind_pipeline(buffer);
		frame.mesh.bind_buffers(buffer);
		for draw_call in frame.draw_calls.iter() {
			buffer.set_dynamic_scissors(vec![draw_call.scissor]);
			let descriptor_set = &self.descriptor_cache[&draw_call.texture_id];
			if let Some(descriptor_set) = descriptor_set.upgrade() {
				self.drawable
					.bind_descriptors(buffer, vec![&descriptor_set]);

				let screen_size: ScreenSizePushConstant = self.screen_size_scaled().into();
				self.drawable
					.push_constant(buffer, flags::ShaderKind::Vertex, 0, &screen_size);
				buffer.draw(
					draw_call.index_count,
					draw_call.first_index,
					1,
					0,
					draw_call.vertex_offset,
				);
			}
		}

		buffer.end_label();

		Ok(())
	}

	fn take_gpu_signals(&mut self) -> Vec<Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}
}

impl Ui {
	fn build_ui(&mut self) -> (Vec<Vertex>, Vec<u32>, Vec<DrawCall>) {
		for (callback, retained) in self.render_callbacks.iter_mut() {
			*retained = callback(&self.context);
		}

		let (output, clipped_shapes) = self.context.end_frame();

		if let Some(egui::output::OpenUrl { url, .. }) = &output.open_url {
			if let Err(err) = webbrowser::open(url) {
				eprintln!("Failed to open url: {}", err);
			}
		}

		// handle clipboard
		if !output.copied_text.is_empty() {
			if let Err(err) = self.clipboard.set_contents(output.copied_text.clone()) {
				eprintln!("Copy/Cut error: {}", err);
			}
		}

		// handle cursor icon
		if self.current_cursor_icon != output.cursor_icon {
			if let Some(arc_window) = self.window_handle.upgrade() {
				let guard = arc_window.write().unwrap();
				if let Some(cursor_icon) = super::conversions::cursor_icon(output.cursor_icon) {
					guard.set_cursor_visible(true);
					guard.set_cursor_icon(cursor_icon);
				} else {
					guard.set_cursor_visible(false);
				}
			}
			self.current_cursor_icon = output.cursor_icon;
		}

		self.raw_input.time = Some(self.last_frame.elapsed().as_secs_f64());
		self.last_frame = Instant::now();

		// Transform draw data into vertex and index buffer data (to upload later)
		let mut vertices: Vec<Vertex> = Vec::new();
		let mut indices: Vec<u32> = Vec::new();
		let mut draw_calls = Vec::new();
		let clipped_meshes = self.context.tessellate(clipped_shapes);
		let mut idx_offset: Vector2<usize> = [0, 0].into();
		for egui::ClippedMesh(rect, mesh) in clipped_meshes {
			if mesh.vertices.is_empty() || mesh.indices.is_empty() {
				continue;
			}
			let item_counts: Vector2<usize> = [mesh.vertices.len(), mesh.indices.len()].into();
			vertices.extend(mesh.vertices.iter().map(|v| {
				Vertex::default()
					.with_position([v.pos.x, v.pos.y].into())
					.with_tex_coord([v.uv.x, v.uv.y].into())
					.with_color(
						[
							(v.color.r() as f32 / 255.0).powf(2.2),
							(v.color.g() as f32 / 255.0).powf(2.2),
							(v.color.b() as f32 / 255.0).powf(2.2),
							(v.color.a() as f32 / 255.0).powf(2.2),
						]
						.into(),
					)
			}));
			indices.extend(mesh.indices.iter());

			let mut min: Vector2<f32> = [rect.min.x, rect.min.y].into();
			min *= self.scale_factor as f32;
			min.x = f32::clamp(min.x, 0.0, self.physical_size.width as f32);
			min.y = f32::clamp(min.y, 0.0, self.physical_size.height as f32);

			let mut max: Vector2<f32> = [rect.max.x, rect.max.y].into();
			max *= self.scale_factor as f32;
			max.x = f32::clamp(max.x, min.x, self.physical_size.width as f32);
			max.y = f32::clamp(max.y, min.y, self.physical_size.height as f32);

			if item_counts.y > 0 {
				draw_calls.push(DrawCall {
					texture_id: mesh.texture_id,
					scissor: Scissor::new(
						structs::Offset2D {
							x: min.x.round() as i32,
							y: min.y.round() as i32,
						},
						structs::Extent2D {
							width: (max.x.round() - min.x) as u32,
							height: (max.y.round() - min.y) as u32,
						},
					),
					index_count: item_counts.y,
					first_index: idx_offset.y,
					vertex_offset: idx_offset.x,
				});
			}

			idx_offset += item_counts;
		}

		(vertices, indices, draw_calls)
	}
}

use crate::{
	engine::{
		self,
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
		Application, Engine, WinitEventListener,
	},
	ui, Editor,
};
use bytemuck::{Pod, Zeroable};
use copypasta::{ClipboardContext, ClipboardProvider};
use egui::{
	math::{pos2, vec2},
	CtxRef, Key,
};
use std::{
	sync::{Arc, RwLock, Weak},
	time::Instant,
};
use winit::event::VirtualKeyCode;

type TextureId = egui::TextureId;

fn winit_to_egui_key_code(key: VirtualKeyCode) -> Option<egui::Key> {
	Some(match key {
		VirtualKeyCode::Down => Key::ArrowDown,
		VirtualKeyCode::Left => Key::ArrowLeft,
		VirtualKeyCode::Right => Key::ArrowRight,
		VirtualKeyCode::Up => Key::ArrowUp,
		VirtualKeyCode::Escape => Key::Escape,
		VirtualKeyCode::Tab => Key::Tab,
		VirtualKeyCode::Back => Key::Backspace,
		VirtualKeyCode::Return => Key::Enter,
		VirtualKeyCode::Space => Key::Space,
		VirtualKeyCode::Insert => Key::Insert,
		VirtualKeyCode::Delete => Key::Delete,
		VirtualKeyCode::Home => Key::Home,
		VirtualKeyCode::End => Key::End,
		VirtualKeyCode::PageUp => Key::PageUp,
		VirtualKeyCode::PageDown => Key::PageDown,
		VirtualKeyCode::Key0 => Key::Num0,
		VirtualKeyCode::Key1 => Key::Num1,
		VirtualKeyCode::Key2 => Key::Num2,
		VirtualKeyCode::Key3 => Key::Num3,
		VirtualKeyCode::Key4 => Key::Num4,
		VirtualKeyCode::Key5 => Key::Num5,
		VirtualKeyCode::Key6 => Key::Num6,
		VirtualKeyCode::Key7 => Key::Num7,
		VirtualKeyCode::Key8 => Key::Num8,
		VirtualKeyCode::Key9 => Key::Num9,
		VirtualKeyCode::A => Key::A,
		VirtualKeyCode::B => Key::B,
		VirtualKeyCode::C => Key::C,
		VirtualKeyCode::D => Key::D,
		VirtualKeyCode::E => Key::E,
		VirtualKeyCode::F => Key::F,
		VirtualKeyCode::G => Key::G,
		VirtualKeyCode::H => Key::H,
		VirtualKeyCode::I => Key::I,
		VirtualKeyCode::J => Key::J,
		VirtualKeyCode::K => Key::K,
		VirtualKeyCode::L => Key::L,
		VirtualKeyCode::M => Key::M,
		VirtualKeyCode::N => Key::N,
		VirtualKeyCode::O => Key::O,
		VirtualKeyCode::P => Key::P,
		VirtualKeyCode::Q => Key::Q,
		VirtualKeyCode::R => Key::R,
		VirtualKeyCode::S => Key::S,
		VirtualKeyCode::T => Key::T,
		VirtualKeyCode::U => Key::U,
		VirtualKeyCode::V => Key::V,
		VirtualKeyCode::W => Key::W,
		VirtualKeyCode::X => Key::X,
		VirtualKeyCode::Y => Key::Y,
		VirtualKeyCode::Z => Key::Z,
		_ => return None,
	})
}

fn winit_to_egui_modifiers(modifiers: winit::event::ModifiersState) -> egui::Modifiers {
	egui::Modifiers {
		alt: modifiers.alt(),
		ctrl: modifiers.ctrl(),
		shift: modifiers.shift(),
		#[cfg(target_os = "macos")]
		mac_cmd: modifiers.logo(),
		#[cfg(target_os = "macos")]
		command: modifiers.logo(),
		#[cfg(not(target_os = "macos"))]
		mac_cmd: false,
		#[cfg(not(target_os = "macos"))]
		command: modifiers.ctrl(),
	}
}

fn winit_to_egui_mouse_button(button: winit::event::MouseButton) -> Option<egui::PointerButton> {
	Some(match button {
		winit::event::MouseButton::Left => egui::PointerButton::Primary,
		winit::event::MouseButton::Right => egui::PointerButton::Secondary,
		winit::event::MouseButton::Middle => egui::PointerButton::Middle,
		_ => return None,
	})
}

/// Convert from [`egui::CursorIcon`] to [`winit::window::CursorIcon`].
fn egui_to_winit_cursor_icon(cursor_icon: egui::CursorIcon) -> Option<winit::window::CursorIcon> {
	Some(match cursor_icon {
		egui::CursorIcon::Default => winit::window::CursorIcon::Default,
		egui::CursorIcon::PointingHand => winit::window::CursorIcon::Hand,
		egui::CursorIcon::ResizeHorizontal => winit::window::CursorIcon::ColResize,
		egui::CursorIcon::ResizeNeSw => winit::window::CursorIcon::NeResize,
		egui::CursorIcon::ResizeNwSe => winit::window::CursorIcon::NwResize,
		egui::CursorIcon::ResizeVertical => winit::window::CursorIcon::RowResize,
		egui::CursorIcon::Text => winit::window::CursorIcon::Text,
		egui::CursorIcon::Grab => winit::window::CursorIcon::Grab,
		egui::CursorIcon::Grabbing => winit::window::CursorIcon::Grabbing,
		egui::CursorIcon::None => return None,
		egui::CursorIcon::ContextMenu => winit::window::CursorIcon::ContextMenu,
		egui::CursorIcon::Help => winit::window::CursorIcon::Help,
		egui::CursorIcon::Progress => winit::window::CursorIcon::Progress,
		egui::CursorIcon::Wait => winit::window::CursorIcon::Wait,
		egui::CursorIcon::Cell => winit::window::CursorIcon::Cell,
		egui::CursorIcon::Crosshair => winit::window::CursorIcon::Crosshair,
		egui::CursorIcon::VerticalText => winit::window::CursorIcon::VerticalText,
		egui::CursorIcon::Alias => winit::window::CursorIcon::Alias,
		egui::CursorIcon::Copy => winit::window::CursorIcon::Copy,
		egui::CursorIcon::Move => winit::window::CursorIcon::Move,
		egui::CursorIcon::NoDrop => winit::window::CursorIcon::NoDrop,
		egui::CursorIcon::NotAllowed => winit::window::CursorIcon::NotAllowed,
		egui::CursorIcon::AllScroll => winit::window::CursorIcon::AllScroll,
		egui::CursorIcon::ZoomIn => winit::window::CursorIcon::ZoomIn,
		egui::CursorIcon::ZoomOut => winit::window::CursorIcon::ZoomOut,
	})
}

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
	ui_elements: Vec<Weak<RwLock<dyn ui::Element>>>,
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
		let strong = Arc::new(RwLock::new(Self::new(engine)?));
		if let Some(mut chain) = engine.render_chain_write() {
			chain.add_render_chain_element(None, &strong)?;
		}
		engine.add_winit_listener(&strong);
		Ok(strong)
	}

	fn new(engine: &mut Engine) -> engine::utility::Result<Ui> {
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
			screen_rect: Some(egui::Rect::from_min_size(
				Default::default(),
				vec2(physical_size.width as f32, physical_size.height as f32) / scale_factor as f32,
			)),
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
			ui_elements: Vec::new(),
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
		size *= self.scale_factor as f32;
		size
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
					self.raw_input.screen_rect = Some(egui::Rect::from_min_size(
						Default::default(),
						vec2(physical_size.width as f32, physical_size.height as f32)
							/ pixels_per_point,
					));
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
					self.raw_input.screen_rect = Some(egui::Rect::from_min_size(
						Default::default(),
						vec2(new_inner_size.width as f32, new_inner_size.height as f32)
							/ pixels_per_point,
					));
				}
				// mouse click
				WindowEvent::MouseInput { state, button, .. } => {
					if let Some(button) = winit_to_egui_mouse_button(*button) {
						self.raw_input.events.push(egui::Event::PointerButton {
							pos: self.mouse_pos,
							button,
							pressed: *state == winit::event::ElementState::Pressed,
							modifiers: winit_to_egui_modifiers(self.modifiers_state),
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
							} else if let Some(key) = winit_to_egui_key_code(virtual_keycode) {
								self.raw_input.events.push(egui::Event::Key {
									key,
									pressed: input.state == winit::event::ElementState::Pressed,
									modifiers: winit_to_egui_modifiers(self.modifiers_state),
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
		"ui"
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
				log::info!(target: crate::EDITOR_LOG, "Refreshing egui font texture");
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
		self.ui_elements
			.retain(|element| element.strong_count() > 0);

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
		for element in self.ui_elements.iter() {
			if let Some(widget) = element.upgrade() {
				if let Ok(mut locked) = widget.write() {
					locked.render(&self.context);
				}
			}
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
				if let Some(cursor_icon) = egui_to_winit_cursor_icon(output.cursor_icon) {
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
			let appended_item_counts: Vector2<usize> =
				[mesh.vertices.len(), mesh.indices.len()].into();
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
				index_count: mesh.indices.len(),
				first_index: idx_offset.x,
				vertex_offset: idx_offset.y,
			});

			idx_offset += appended_item_counts;
		}

		(vertices, indices, draw_calls)
	}
}

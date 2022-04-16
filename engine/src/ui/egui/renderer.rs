use super::Element;
use crate::{
	graphics::{
		self,
		chain::{operation::RequiresRecording, Operation},
		command, descriptor, flags, pipeline,
		procedure::Phase,
		structs,
		types::{Vec2, Vec4},
		utility::Scissor,
		utility::{BuildFromDevice, NameableBuilder},
		vertex_object, Chain, DescriptorCache, Drawable, ImageCache, Mesh,
	},
	math::nalgebra::{Vector2, Vector4},
	window::Window,
	WinitEventListener,
};
use bytemuck::{Pod, Zeroable};
use copypasta::{ClipboardContext, ClipboardProvider};
use egui::{
	emath::{pos2, vec2},
	Context, ImageData,
};
use std::{
	sync::{Arc, RwLock, Weak},
	time::Instant,
};
use vulkan_rs::structs::Extent2D;
use winit::event::VirtualKeyCode;

type TextureId = egui::TextureId;

pub struct Ui {
	physical_size: winit::dpi::PhysicalSize<u32>,
	scale_factor: f64,

	context: Context,
	raw_input: egui::RawInput,
	mouse_pos: egui::Pos2,
	modifiers_state: winit::event::ModifiersState,
	clipboard: ClipboardContext,
	current_cursor_icon: egui::CursorIcon,
	pending_update: PendingUpdate,

	frames: Vec<Frame>,
	descriptor_cache: DescriptorCache<TextureId>,
	image_cache: ImageCache<TextureId>,
	drawable: Drawable,
	render_callbacks: Vec<(Box<dyn Fn(&egui::Context) -> bool>, /*retained*/ bool)>,
	/// The timestamp of the first frame render call.
	start_time: Option<Instant>,
	window_handle: Weak<RwLock<winit::window::Window>>,
}

struct Frame {
	draw_calls: Vec<DrawCall>,
	mesh: Mesh<u32, Vertex>,
	/// TextureIds that were marked as unused/free the last time this frame was submitted
	unused_texture_ids: Vec<TextureId>,
}

#[derive(Default)]
struct PendingUpdate {
	shapes: Vec<egui::epaint::ClippedShape>,
	needs_repaint: bool,
	unused_texture_ids: Vec<TextureId>,
}
impl PendingUpdate {
	fn take(&mut self) -> Self {
		let needs_repaint = self.needs_repaint;
		self.needs_repaint = false;
		Self {
			shapes: self.shapes.drain(..).collect(),
			needs_repaint,
			unused_texture_ids: self.unused_texture_ids.drain(..).collect(),
		}
	}
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
	pub fn create(window: &Window, phase: &Arc<Phase>) -> anyhow::Result<Arc<RwLock<Self>>> {
		let strong = Arc::new(RwLock::new(Self::new(window)?));
		if let Ok(mut chain) = window.graphics_chain().write() {
			chain.add_operation(phase, Arc::downgrade(&strong))?;
		}
		Ok(strong)
	}

	fn get_screen_rect(
		physical_size: &winit::dpi::PhysicalSize<u32>,
		scale_factor: f64,
	) -> egui::Rect {
		egui::Rect::from_min_size(
			Default::default(),
			vec2(physical_size.width as f32, physical_size.height as f32) / scale_factor as f32,
		)
	}

	fn new(window: &Window) -> anyhow::Result<Ui> {
		let window_handle = window.unwrap();
		let (physical_size, scale_factor) = window.read_size();

		// Create context
		let context = Context::default();

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
		let logical = window.graphics_chain().read().unwrap().logical()?;

		Ok(Ui {
			physical_size,
			scale_factor,

			context,
			raw_input,
			mouse_pos,
			modifiers_state,
			clipboard,
			current_cursor_icon: egui::CursorIcon::None,
			pending_update: PendingUpdate::default(),

			window_handle,
			start_time: None,
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
					.build(&logical)?,
			),
			frames: Vec::new(),
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
		F: 'static + Fn(&egui::Context) -> bool,
	{
		self.render_callbacks.push((Box::new(callback), true));
	}

	pub fn context_mut(&mut self) -> &mut Context {
		&mut self.context
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
					self.raw_input.screen_rect = Some(Self::get_screen_rect(
						&physical_size,
						pixels_per_point as f64,
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
					self.raw_input.screen_rect = Some(Self::get_screen_rect(
						&new_inner_size,
						pixels_per_point as f64,
					));
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
						// NOTE: EGui does not handle horizontal scrolling, it has to be provided a scroll event that uses the X axis.
						// It was unknown (at time of writing) if LineDelta provides this.
						self.raw_input
							.events
							.push(egui::Event::Scroll(vec2(*x, *y) * line_height));
					}
					winit::event::MouseScrollDelta::PixelDelta(delta) => {
						self.raw_input
							.events
							.push(egui::Event::Scroll(vec2(delta.x as f32, delta.y as f32)));
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

impl Operation for Ui {
	fn initialize(&mut self, chain: &Chain) -> anyhow::Result<()> {
		use crate::{Application, EngineApp};
		let vertex = EngineApp::get_asset_id("shaders/debug/egui/vertex");
		let fragment = EngineApp::get_asset_id("shaders/debug/egui/fragment");

		self.drawable.add_shader(&vertex)?;
		self.drawable.add_shader(&fragment)?;
		self.drawable.create_shaders(&chain.logical()?)?;

		self.drawable.add_push_constant_range(
			pipeline::PushConstantRange::default()
				.with_stage(flags::ShaderKind::Vertex)
				.with_size(std::mem::size_of::<ScreenSizePushConstant>()),
		);

		for i in 0..chain.view_count() {
			self.frames.push(Frame {
				mesh: Mesh::<u32, Vertex>::new(
					format!("EditorUI.Frame{}.Mesh", i),
					&chain.allocator()?,
					10,
				)?,
				draw_calls: Vec::new(),
				unused_texture_ids: Vec::new(),
			});
		}

		Ok(())
	}

	fn construct(&mut self, chain: &Chain, subpass_index: usize) -> anyhow::Result<()> {
		use flags::blend::prelude::*;
		use pipeline::state::*;
		self.drawable.create_pipeline(
			&chain.logical()?,
			vec![self.descriptor_cache.layout()],
			pipeline::Pipeline::builder()
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::dynamic(1, 1))
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
				.with_dynamic_state(flags::DynamicState::VIEWPORT)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			chain.render_pass(),
			subpass_index,
		)?;
		Ok(())
	}

	fn deconstruct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		self.drawable.destroy_pipeline()?;
		Ok(())
	}

	fn prepare_for_frame(&mut self, chain: &Chain) -> anyhow::Result<()> {
		match self.start_time {
			// Save the time that the first frame was rendered/processed at.
			None => {
				self.start_time = Some(Instant::now());
			}
			// Each successive frame should update the egui time to be the amount of time passed since the first frame
			Some(time) => {
				self.raw_input.time = Some(time.elapsed().as_secs_f64());
			}
		}

		self.context.begin_frame(self.raw_input.take());
		for (callback, retained) in self.render_callbacks.iter_mut() {
			*retained = callback(&self.context);
		}
		let egui::output::FullOutput {
			platform_output,
			shapes,
			textures_delta,
			needs_repaint,
		} = self.context.end_frame();

		let unused_texture_ids = {
			let egui::TexturesDelta { set, free } = textures_delta;
			self.insert_image_operations(set.into_iter())?;
			self.process_image_ops(chain)?;
			free
		};

		self.handle_url_request(&platform_output.open_url);
		self.update_clipboard(&platform_output.copied_text);
		self.update_cursor_icon(platform_output.cursor_icon);

		self.pending_update = PendingUpdate {
			shapes,
			needs_repaint,
			unused_texture_ids,
		};

		Ok(())
	}

	fn prepare_for_submit(
		&mut self,
		chain: &graphics::Chain,
		frame_image: usize,
	) -> anyhow::Result<RequiresRecording> {
		self.render_callbacks.retain(|(_, retained)| *retained);

		for id in self.frames[frame_image].unused_texture_ids.drain(..) {
			// remove from cache and drop in this stackframe
			let _ = self.image_cache.remove(&id);
		}

		// Take the last pending update (provided by `prepare_for_frame`).
		let update = self.pending_update.take();
		// Prepare the frame with the pending update
		{
			let (vertices, indices, draw_calls) = self.tesselate_shapes(update.shapes);
			let frame = &mut self.frames[frame_image];
			frame
				.mesh
				.write(&vertices, &indices, chain, chain.signal_sender())?;
			frame.draw_calls = draw_calls;
			frame.unused_texture_ids = update.unused_texture_ids.clone();
		}

		Ok(match update.needs_repaint {
			true => RequiresRecording::AllFrames,
			false => RequiresRecording::NotRequired,
		})
	}

	fn record(&mut self, buffer: &mut command::Buffer, buffer_index: usize) -> anyhow::Result<()> {
		let frame = &self.frames[buffer_index];

		buffer.begin_label("Draw:EditorUI", graphics::debug::LABEL_COLOR_DRAW);

		self.drawable.bind_pipeline(buffer);
		frame.mesh.bind_buffers(buffer);
		buffer.set_dynamic_viewport(
			0,
			vec![vulkan_rs::utility::Viewport::default().set_size(Extent2D {
				width: self.physical_size.width,
				height: self.physical_size.height,
			})],
		);
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
}

impl Ui {
	#[profiling::function]
	fn insert_image_operations<T>(&mut self, updates: T) -> anyhow::Result<()>
	where
		T: std::iter::Iterator<Item = (TextureId, egui::epaint::image::ImageDelta)>,
	{
		for (id, delta) in updates {
			if !self.image_cache.contains(&id) {
				use flags::format::prelude::*;
				// The first time the id is encountered, we need the full size of the image
				assert!(!delta.pos.is_some());

				self.image_cache.insert_operation(
					id,
					graphics::PendingOperation::Create(
						graphics::PendingImage::default()
							.with_name(format!(
								"EGui.Texture.{}",
								match id {
									TextureId::Managed(id) => format!("Managed({id})"),
									TextureId::User(id) => format!("User({id})"),
								}
							))
							.with_size(delta.image.size().into())
							.with_format(flags::format::format(&[R, G, B, A], Bit8, UnsignedNorm))
							.init_sampler(|sampler| {
								sampler.set_magnification(flags::Filter::LINEAR);
								sampler.set_minification(flags::Filter::LINEAR);
								sampler.set_address_modes(
									[flags::SamplerAddressMode::CLAMP_TO_EDGE; 3],
								);
							}),
					),
				);
			}

			// Convert the delta into its srgba pixels
			let size = delta.image.size().into();
			let srgba_pixels = match delta.image {
				ImageData::Color(image) => image
					.pixels
					.iter()
					.flat_map(|c| c.to_array())
					.collect::<Vec<_>>(),
				ImageData::Alpha(image) => image
					.srgba_pixels(1.0)
					.flat_map(|c| c.to_array())
					.collect::<Vec<_>>(),
			};

			// Write a parial or whole update to a texture that may or may not be pending
			self.image_cache.insert_operation(
				id,
				graphics::PendingOperation::Write(
					graphics::ImageBinary::from(size, srgba_pixels)
						.with_offset(delta.pos.map(|xy| xy.into())),
				),
			);
		}
		Ok(())
	}

	fn process_image_ops(&mut self, chain: &Chain) -> anyhow::Result<()> {
		let new_images = self
			.image_cache
			.process_operations(chain, chain.signal_sender())?;
		for (image_id, image_name) in new_images.into_iter() {
			use descriptor::update::*;
			let cached_image = &self.image_cache[&image_id];
			let descriptor_set = self.descriptor_cache.insert(
				image_id,
				image_name.map(|v| format!("EditorUI.{}", v)),
				chain.persistent_descriptor_pool(),
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
				.apply(&*chain.logical()?);
		}
		Ok(())
	}

	fn handle_url_request(&self, request: &Option<egui::output::OpenUrl>) {
		if let Some(egui::output::OpenUrl { url, .. }) = request {
			if let Err(err) = webbrowser::open(url) {
				eprintln!("Failed to open url: {}", err);
			}
		}
	}

	fn update_clipboard(&mut self, copied_text: &String) {
		if !copied_text.is_empty() {
			if let Err(err) = self.clipboard.set_contents(copied_text.clone()) {
				eprintln!("Copy/Cut error: {}", err);
			}
		}
	}

	fn update_cursor_icon(&mut self, cursor_icon: egui::output::CursorIcon) {
		if self.current_cursor_icon != cursor_icon {
			if let Some(arc_window) = self.window_handle.upgrade() {
				let guard = arc_window.write().unwrap();
				if let Some(cursor_icon) = super::conversions::cursor_icon(cursor_icon) {
					guard.set_cursor_visible(true);
					guard.set_cursor_icon(cursor_icon);
				} else {
					guard.set_cursor_visible(false);
				}
			}
			self.current_cursor_icon = cursor_icon;
		}
	}

	#[profiling::function]
	fn tesselate_shapes(
		&mut self,
		shapes: Vec<egui::epaint::ClippedShape>,
	) -> (Vec<Vertex>, Vec<u32>, Vec<DrawCall>) {
		// Transform draw data into vertex and index buffer data (to upload later)
		let mut vertices: Vec<Vertex> = Vec::new();
		let mut indices: Vec<u32> = Vec::new();
		let mut draw_calls = Vec::new();
		let clipped_meshes = self.context.tessellate(shapes);
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

			idx_offset += item_counts;
		}

		(vertices, indices, draw_calls)
	}
}

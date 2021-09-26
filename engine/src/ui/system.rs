use crate::{
	asset,
	graphics::{
		self, command, flags,
		font::Font,
		pipeline, structs,
		structs::{Extent2D, Offset2D},
		utility::Scissor,
		Drawable, Texture,
	},
	input,
	math::nalgebra::{Matrix4, Point2, Vector2, Vector4},
	ui::{mesh::*, *},
	utility::{self, AnyError, VoidResult},
	EngineSystem, WinitEventListener,
};
use enumset::EnumSet;
use raui::renderer::tesselate::prelude::*;
use std::{
	any::{Any, TypeId},
	collections::HashMap,
	sync,
};

/// The types of shaders used by the [`ui system`](System).
pub enum SystemShader {
	/// The shader used in the [`vertex stage`](flags::ShaderKind::Vertex) of the text renderer.
	TextVertex,
	/// The shader used in the [`fragment stage`](flags::ShaderKind::Fragment) of the text renderer.
	TextFragment,
	/// The shader used in the [`vertex stage`](flags::ShaderKind::Vertex) of the image & colored-rect renderers.
	MeshVertex,
	/// The shader used in the [`fragment stage`](flags::ShaderKind::Fragment) of the colored-rect renderer.
	MeshSimpleFragment,
	/// The shader used in the [`fragment stage`](flags::ShaderKind::Fragment) of the image renderer.
	MeshImageFragment,
}

pub type UiContext = sync::Arc<sync::RwLock<dyn Any + 'static + Send + Sync>>;
pub struct ContextContainer {
	items: HashMap<TypeId, UiContext>,
}
impl Default for ContextContainer {
	fn default() -> Self {
		Self {
			items: HashMap::new(),
		}
	}
}
impl ContextContainer {
	pub fn get<T: 'static>(&self) -> Option<ContextGuard<T>> {
		self.items
			.get(&TypeId::of::<T>())
			.map(|x| {
				x.read()
					.map(|guard| ContextGuard(guard, std::marker::PhantomData))
					.ok()
			})
			.flatten()
	}
	pub fn insert<T>(&mut self, item: sync::Arc<sync::RwLock<T>>)
	where
		T: 'static + Send + Sync,
	{
		self.items.insert(TypeId::of::<T>(), item);
	}
}
pub struct ContextGuard<'a, T>(
	sync::RwLockReadGuard<'a, dyn Any + 'static + Send + Sync>,
	std::marker::PhantomData<T>,
);
impl<'a, T: 'static> std::ops::Deref for ContextGuard<'a, T> {
	type Target = T;
	fn deref(&self) -> &Self::Target {
		&(*self.0).downcast_ref::<T>().unwrap()
	}
}

/// Handles the rendering of the UI widgets to the screen.
/// Also updates and processes the UI widgets via the ECS system.
pub struct System {
	draw_calls_by_frame: Vec<Vec<DrawCall>>,

	pending_gpu_signals: Vec<sync::Arc<command::Semaphore>>,
	atlas_mapping: HashMap<String, (String, Rect)>,
	image_sizes: HashMap<String, Vec2>,

	text_widgets: Vec<HashMap<WidgetId, text::WidgetData>>,
	text: text::DataPipeline,
	image: image::DataPipeline,
	colored_area: Drawable,
	frame_meshes: Vec<Mesh>,

	mouse_position_unnormalized: Point2<f32>,
	resolution: Vector2<f32>,
	keyboard_modifiers: EnumSet<input::source::KeyModifier>,
	interactions: DefaultInteractionsEngine,
	contexts: ContextContainer,
	application: Application,
}

#[derive(Debug, Clone)]
enum DrawCall {
	Text(WidgetId),
	Range(std::ops::Range<usize>),
	Texture(String, std::ops::Range<usize>),
	PushClip(Scissor),
	PopClip(),
}

impl System {
	/// Constructs a ui rendering system for the provided render chain.
	pub fn new(
		render_chain: &sync::Arc<sync::RwLock<graphics::RenderChain>>,
	) -> utility::Result<Self> {
		let mut application = Application::new();
		application.setup(widget::setup);

		let mut interactions = DefaultInteractionsEngine::new();
		interactions.deselect_when_no_button_found = true;

		let chain_read = render_chain.read().unwrap();
		Ok(Self {
			application,
			contexts: ContextContainer::default(),
			interactions,
			keyboard_modifiers: EnumSet::empty(),
			resolution: [0.0, 0.0].into(),
			mouse_position_unnormalized: [0.0, 0.0].into(),
			frame_meshes: Vec::new(),
			colored_area: Drawable::default().with_name("UI.ColoredArea"),
			image: image::DataPipeline::new(&chain_read)?,
			text: text::DataPipeline::new(&chain_read)?,
			text_widgets: Vec::new(),
			atlas_mapping: HashMap::new(),
			image_sizes: HashMap::new(),
			pending_gpu_signals: Vec::new(),
			draw_calls_by_frame: Vec::new(),
		})
	}

	pub fn with_tree(mut self, tree: WidgetNode) -> Self {
		self.apply_tree(tree);
		self
	}

	pub fn with_tree_root(mut self, component: WidgetComponent) -> Self {
		self.apply_tree(WidgetNode::Component(component));
		self
	}

	/// Set the ui widget tree to update and render.
	pub fn apply_tree(&mut self, tree: WidgetNode) {
		self.application.apply(tree);
	}

	pub fn with_context<T>(mut self, item: sync::Arc<sync::RwLock<T>>) -> Self
	where
		T: 'static + Send + Sync,
	{
		self.contexts.insert(item);
		self
	}

	fn mapping(&self) -> CoordsMapping {
		CoordsMapping::new(Rect {
			left: 0.0,
			right: self.resolution.x,
			top: 0.0,
			bottom: self.resolution.y,
		})
	}

	fn mouse_position(&self) -> Vec2 {
		self.mapping().real_to_virtual_vec2(
			Vec2 {
				x: self.mouse_position_unnormalized.x,
				y: self.mouse_position_unnormalized.y,
			},
			false,
		)
	}

	/// Render widgets into interleaved tesselation mesh & batches.
	#[profiling::function]
	fn tesselate(&self, coord_mapping: &CoordsMapping) -> Option<Tesselation> {
		let mut renderer = TesselateRenderer::with_capacity(
			TesselationVerticesFormat::Interleaved,
			(),
			&self.atlas_mapping,
			&self.image_sizes,
			64,
		);
		self.application
			.render(&coord_mapping, &mut renderer)
			.map(|success| success.optimized_batches())
			.ok()
	}

	pub fn with_engine_shaders(mut self) -> Result<Self, utility::AnyError> {
		self.initialize_engine_shaders()?;
		Ok(self)
	}

	/// Initializes the ui system with shaders from [`EngineApp`](crate::EngineApp),
	/// instead of manually applying shaders for each [`SystemShader`] type.
	pub fn initialize_engine_shaders(&mut self) -> VoidResult {
		use crate::{Application, EngineApp};
		log::info!(target: LOG, "Initializing engine shaders");
		self.add_shader(
			SystemShader::TextVertex,
			&EngineApp::get_asset_id("shaders/ui/text/vertex"),
		)?;
		self.add_shader(
			SystemShader::TextFragment,
			&EngineApp::get_asset_id("shaders/ui/text/fragment"),
		)?;
		self.add_shader(
			SystemShader::MeshVertex,
			&EngineApp::get_asset_id("shaders/ui/mesh/vertex"),
		)?;
		self.add_shader(
			SystemShader::MeshSimpleFragment,
			&EngineApp::get_asset_id("shaders/ui/mesh/simple_fragment"),
		)?;
		self.add_shader(
			SystemShader::MeshImageFragment,
			&EngineApp::get_asset_id("shaders/ui/mesh/image_fragment"),
		)?;
		Ok(())
	}

	/// Adds a shader to the ui system so that the various kinda of meshes can render.
	/// The system must be provided with one of each system shader before it can run properly.
	pub fn add_shader(&mut self, key: SystemShader, id: &asset::Id) -> VoidResult {
		match key {
			SystemShader::TextVertex | SystemShader::TextFragment => self.text.add_shader(id),
			SystemShader::MeshVertex => {
				self.colored_area.add_shader(id)?;
				self.image.add_shader(id)?;
				Ok(())
			}
			SystemShader::MeshSimpleFragment => self.colored_area.add_shader(id),
			SystemShader::MeshImageFragment => self.image.add_shader(id),
		}
	}

	pub fn with_font(mut self, id: &asset::Id) -> Result<Self, utility::AnyError> {
		self.add_font(id)?;
		Ok(self)
	}

	pub fn with_all_fonts(mut self) -> Result<Self, utility::AnyError> {
		let library = crate::asset::Library::read();
		let font_asset_ids = library.get_ids_of_type::<Font>();
		if let Some(asset_ids) = font_asset_ids {
			for id in asset_ids {
				self.add_font(id)?;
			}
		}
		Ok(self)
	}

	/// Adds a font to the text rendering system.
	/// Fonts must be registered/added before they can be used in a widget,
	/// but can be added at any point in the lifecycle of the renderer.
	pub fn add_font(&mut self, id: &asset::Id) -> VoidResult {
		let asset = asset::Loader::load_sync(&id)?.downcast::<Font>().unwrap();
		log::info!(
			target: LOG,
			"Adding font '{}' with width-edge <{},{}>",
			id.as_string(),
			asset.width_edge().x,
			asset.width_edge().y,
		);
		self.text.add_pending(id.name(), asset);
		Ok(())
	}

	pub fn with_texture(mut self, id: &asset::Id) -> Result<Self, utility::AnyError> {
		self.add_texture(id)?;
		Ok(self)
	}

	/// Adds a texture to the image rendering system.
	/// Images must be registered/added before they can be used in a widget,
	/// but can be added at any point in the lifecycle of the renderer.
	pub fn add_texture(&mut self, id: &asset::Id) -> VoidResult {
		if let Ok(asset) = asset::Loader::load_sync(&id) {
			let texture = asset.downcast::<Texture>().unwrap();
			log::info!(target: LOG, "Adding texture '{}'", id.as_string());
			self.image_sizes.insert(
				id.name(),
				[texture.size().x as f32, texture.size().y as f32].into(),
			);
			self.image.add_pending(id, texture);
		} else {
			log::error!("Failed to load texture asset {}", id);
		}
		Ok(())
	}

	pub fn attach_system(
		self,
		engine: &mut crate::Engine,
		subpass_id: Option<String>,
	) -> Result<sync::Arc<sync::RwLock<Self>>, utility::AnyError> {
		let system = sync::Arc::new(sync::RwLock::new(self));
		engine.add_system(&system);
		engine.add_winit_listener(&system);
		if let Some(mut chain_write) = engine
			.render_chain()
			.map(|chain| chain.write().ok())
			.flatten()
		{
			chain_write.add_render_chain_element(subpass_id, &system)?;
		}
		Ok(system)
	}

	fn interact(&mut self, interaction: Interaction) {
		self.interactions.interact(interaction);
	}
}

impl WinitEventListener for System {
	fn on_event(&mut self, event: &winit::event::Event<()>) {
		use crate::input::source::{Key, MouseButton};
		use std::convert::TryFrom;
		use winit::event::{
			DeviceEvent, ElementState, KeyboardInput, MouseScrollDelta, WindowEvent,
		};
		match event {
			winit::event::Event::WindowEvent {
				event: WindowEvent::CursorMoved { position, .. },
				..
			} => {
				self.mouse_position_unnormalized = [position.x as f32, position.y as f32].into();
				self.interact(Interaction::PointerMove(self.mouse_position()));
			}
			winit::event::Event::DeviceEvent {
				event:
					DeviceEvent::MouseWheel {
						delta: MouseScrollDelta::LineDelta(horizontal, vertical),
					},
				..
			} => {
				let single_scroll_units = Vec2 { x: 10.0, y: 10.0 };
				self.interact(Interaction::Navigate(NavSignal::Jump(NavJump::Scroll(
					NavScroll::Units(
						Vec2 {
							x: -single_scroll_units.x * (*horizontal),
							y: -single_scroll_units.y * (*vertical),
						},
						true,
					),
				))));
			}
			winit::event::Event::DeviceEvent {
				event: DeviceEvent::Button { button, state },
				..
			} => {
				if let Some(pointer_button) = match MouseButton::try_from(*button) {
					Ok(MouseButton::Left) => Some(PointerButton::Trigger),
					Ok(MouseButton::Right) => Some(PointerButton::Context),
					_ => None,
				} {
					self.interact(match state {
						ElementState::Pressed => {
							Interaction::PointerDown(pointer_button, self.mouse_position())
						}
						ElementState::Released => {
							Interaction::PointerUp(pointer_button, self.mouse_position())
						}
					});
				}
			}
			winit::event::Event::WindowEvent {
				event: WindowEvent::ModifiersChanged(modifiers_state),
				..
			} => {
				self.keyboard_modifiers = EnumSet::empty();
				for (active, modifier) in [
					(modifiers_state.shift(), input::source::KeyModifier::Shift),
					(modifiers_state.ctrl(), input::source::KeyModifier::Control),
					(modifiers_state.alt(), input::source::KeyModifier::Alt),
					(modifiers_state.logo(), input::source::KeyModifier::Platform),
				] {
					if active {
						self.keyboard_modifiers.insert(modifier);
					}
				}
			}
			winit::event::Event::DeviceEvent {
				event:
					DeviceEvent::Key(KeyboardInput {
						state: ElementState::Pressed,
						virtual_keycode: Some(keycode),
						..
					}),
				..
			} => {
				if let Ok(key) = Key::try_from(*keycode) {
					if let Some(signals) = match self.interactions.focused_text_input() {
						Some(_) => match key {
							Key::Left => {
								Some(vec![NavSignal::TextChange(NavTextChange::MoveCursorLeft)])
							}
							Key::Right => {
								Some(vec![NavSignal::TextChange(NavTextChange::MoveCursorRight)])
							}
							Key::Home => {
								Some(vec![NavSignal::TextChange(NavTextChange::MoveCursorStart)])
							}
							Key::End => {
								Some(vec![NavSignal::TextChange(NavTextChange::MoveCursorEnd)])
							}
							Key::Back => {
								Some(vec![NavSignal::TextChange(NavTextChange::DeleteLeft)])
							}
							Key::Delete => {
								Some(vec![NavSignal::TextChange(NavTextChange::DeleteRight)])
							}
							Key::Return | Key::NumpadEnter => {
								Some(vec![NavSignal::TextChange(NavTextChange::NewLine)])
							}
							Key::Escape => Some(vec![NavSignal::FocusTextInput(().into())]),
							key => match key.to_string(&self.keyboard_modifiers) {
								Some(string) => {
									let mut signals = Vec::new();
									for c in string.chars() {
										signals.push(NavSignal::TextChange(
											NavTextChange::InsertCharacter(c),
										));
									}
									Some(signals)
								}
								None => None,
							},
						},
						None => match key {
							Key::W | Key::Up => Some(vec![NavSignal::Up]),
							Key::A | Key::Left => Some(vec![NavSignal::Left]),
							Key::S | Key::Down => Some(vec![NavSignal::Down]),
							Key::D | Key::Right => Some(vec![NavSignal::Right]),
							Key::Return | Key::NumpadEnter | Key::Space => {
								Some(vec![NavSignal::Accept(true)])
							}
							Key::Escape => Some(vec![NavSignal::Cancel(true)]),
							_ => None,
						},
					} {
						for signal in signals.into_iter() {
							self.interact(Interaction::Navigate(signal));
						}
					}
				}
			}
			winit::event::Event::DeviceEvent {
				event:
					DeviceEvent::Key(KeyboardInput {
						state: ElementState::Released,
						virtual_keycode: Some(keycode),
						..
					}),
				..
			} => {
				if let Ok(key) = Key::try_from(*keycode) {
					if let Some(signal) = match self.interactions.focused_text_input() {
						Some(_) => None,
						None => match key {
							Key::Return | Key::NumpadEnter | Key::Space => {
								Some(NavSignal::Accept(false))
							}
							Key::Escape => Some(NavSignal::Cancel(false)),
							_ => None,
						},
					} {
						self.interact(Interaction::Navigate(signal));
					}
				}
			}
			_ => {}
		}
	}
}

impl EngineSystem for System {
	#[profiling::function]
	fn update(&mut self, _: std::time::Duration) {
		let mapping = self.mapping();
		self.application
			.forced_process_with_context(ProcessContext::new().insert(&self.contexts));
		let _res = self.application.layout(&mapping, &mut DefaultLayoutEngine);
		let _res = self.application.interact(&mut self.interactions);
	}
}

impl graphics::RenderChainElement for System {
	fn name(&self) -> &'static str {
		"render-ui"
	}

	#[profiling::function]
	fn initialize_with(
		&mut self,
		render_chain: &mut graphics::RenderChain,
	) -> Result<Vec<sync::Arc<command::Semaphore>>, AnyError> {
		self.draw_calls_by_frame
			.resize(render_chain.frame_count(), Vec::new());
		self.colored_area.create_shaders(&render_chain)?;
		self.image.create_shaders(&render_chain)?;
		self.text.create_shaders(&render_chain)?;
		self.pending_gpu_signals
			.append(&mut self.text.create_pending_font_atlases(&render_chain)?);
		for i in 0..render_chain.frame_count() {
			self.text_widgets.push(HashMap::new());
			self.frame_meshes.push(Mesh::new(
				format!("UI.Frame{}.Mesh", i),
				&render_chain.allocator(),
				10,
			)?);
		}
		Ok(self.take_gpu_signals())
	}

	#[profiling::function]
	fn destroy_render_chain(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> Result<(), AnyError> {
		self.colored_area.destroy_pipeline(render_chain)?;
		self.image.destroy_pipeline(render_chain)?;
		self.text.destroy_render_chain(render_chain)?;
		Ok(())
	}

	#[profiling::function]
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: &Vector2<f32>,
		subpass_id: &Option<String>,
	) -> Result<(), AnyError> {
		use pipeline::state::*;
		self.resolution = *resolution;
		self.colored_area.create_pipeline(
			render_chain,
			vec![],
			pipeline::Pipeline::builder()
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::from(structs::Extent2D {
					width: resolution.x as u32,
					height: resolution.y as u32,
				}))
				.set_color_blending(
					color_blend::ColorBlend::default()
						.add_attachment(color_blend::Attachment::default()),
				)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			subpass_id,
		)?;
		self.image
			.create_pipeline(render_chain, resolution, subpass_id)?;
		self.text
			.on_render_chain_constructed(render_chain, resolution, subpass_id)?;
		Ok(())
	}

	fn preframe_update(&mut self, render_chain: &graphics::RenderChain) -> Result<(), AnyError> {
		self.pending_gpu_signals
			.append(&mut self.image.create_pending_images(&render_chain)?);
		self.pending_gpu_signals
			.append(&mut self.text.create_pending_font_atlases(&render_chain)?);
		Ok(())
	}

	fn take_gpu_signals(&mut self) -> Vec<sync::Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}

	/// Update the data (like uniforms) for a given frame -
	/// Or in the case of the UI Render, record changes to the secondary command buffer.
	#[profiling::function]
	fn prerecord_update(
		&mut self,
		render_chain: &graphics::RenderChain,
		_buffer: &command::Buffer,
		frame: usize,
		resolution: &Vector2<f32>,
	) -> Result<bool, AnyError> {
		let mapping = self.mapping();
		// Drain the existing widgets
		let mut retained_text_widgets: HashMap<WidgetId, text::WidgetData> =
			self.text_widgets[frame].drain().collect();
		let mut draw_calls = Vec::new();

		// Garuntee that there will always be a scissor clip available for the recording to use
		draw_calls.push(DrawCall::PushClip(Scissor::new(
			Offset2D::default(),
			Extent2D {
				width: resolution.x as u32,
				height: resolution.y as u32,
			},
		)));

		if let Some(tesselation) = self.tesselate(&mapping) {
			let (vertices, indices) =
				Vertex::create_interleaved_buffer_data(&tesselation, resolution);
			let mut mesh_gpu_signals =
				self.frame_meshes[frame].write(&vertices, &indices, &render_chain)?;
			self.pending_gpu_signals.append(&mut mesh_gpu_signals);

			for batch in tesselation.batches {
				match batch {
					Batch::None | Batch::FontTriangles(_, _, _) => {}
					Batch::ColoredTriangles(range) => {
						draw_calls.push(DrawCall::Range(range));
					}
					Batch::ImageTriangles(texture_id, range) => {
						if self.image.has_image(&texture_id) {
							draw_calls.push(DrawCall::Texture(texture_id, range));
						} else {
							log::warn!(
								"Texture for id \"{}\" has not been added to the ui render system.",
								texture_id
							);
						}
					}
					Batch::ExternalText(widget_id, text) => {
						let (widget_data, mut gpu_signals) = self.text.update_or_create(
							render_chain,
							resolution,
							&widget_id,
							text,
							retained_text_widgets.remove(&widget_id),
						)?;
						self.text_widgets[frame].insert(widget_id.clone(), widget_data);
						self.pending_gpu_signals.append(&mut gpu_signals);
						draw_calls.push(DrawCall::Text(widget_id));
					}
					Batch::ClipPush(clip) => {
						let column_major = Matrix4::<f32>::from_vec_generic(
							nalgebra::Const::<4>,
							nalgebra::Const::<4>,
							clip.matrix.to_vec(),
						);
						let clip_vec: Vector2<f32> = [clip.box_size.x, clip.box_size.y].into();
						let transform = |mask: Vector2<f32>| -> Vector2<f32> {
							let v2 = clip_vec.component_mul(&mask);
							let v4: Vector4<f32> = [v2.x, v2.y, 0.0, 1.0].into();
							(column_major * v4).xy()
						};

						let tl = transform([0.0, 0.0].into());
						let tr = transform([1.0, 0.0].into());
						let bl = transform([0.0, 1.0].into());
						let br = transform([1.0, 1.0].into());

						let min: Vector2<f32> = [
							tl.x.min(tr.x).min(bl.x).min(br.x),
							tl.y.min(tr.y).min(bl.y).min(br.y),
						]
						.into();
						let max: Vector2<f32> = [
							tl.x.max(tr.x).max(bl.x).max(br.x),
							tl.y.max(tr.y).max(bl.y).max(br.y),
						]
						.into();
						let size = max - min;

						draw_calls.push(DrawCall::PushClip(Scissor::new(
							Offset2D {
								x: min.x as i32,
								y: max.y as i32,
							},
							Extent2D {
								width: size.x as u32,
								height: size.y as u32,
							},
						)));
					}
					Batch::ClipPop => {
						draw_calls.push(DrawCall::PopClip());
					}
				}
			}
		}

		self.draw_calls_by_frame[frame] = draw_calls;

		Ok(true) // the ui uses immediate mode, and therefore requires re-recording every frame
	}

	/// Record to the primary command buffer for a given frame
	#[profiling::function]
	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> Result<(), AnyError> {
		use graphics::debug;
		let mut clips = Vec::new();
		buffer.begin_label("Draw:UI", debug::LABEL_COLOR_DRAW);
		for call in self.draw_calls_by_frame[frame].iter() {
			match call {
				DrawCall::PushClip(scissor) => clips.push(scissor),
				DrawCall::PopClip() => {
					clips.pop();
				}

				DrawCall::Text(widget_id) => {
					let widget_data = &self.text_widgets[frame][widget_id];
					if let Some(name) = widget_data.name().as_ref() {
						buffer
							.begin_label(format!("Draw:UI/Text {}", name), debug::LABEL_COLOR_DRAW);
					}
					buffer.set_dynamic_scissors(vec![**clips.last().unwrap()]);
					self.text.record_to_buffer(buffer, widget_data)?;
					if widget_data.name().is_some() {
						buffer.end_label();
					}
				}
				DrawCall::Range(range) => {
					buffer.begin_label("Draw:UI/ColoredArea", debug::LABEL_COLOR_DRAW);
					buffer.set_dynamic_scissors(vec![**clips.last().unwrap()]);
					self.colored_area.bind_pipeline(buffer);
					self.frame_meshes[frame].bind_buffers(buffer);
					buffer.draw(range.end - range.start, range.start, 1, 0, 0);
					buffer.end_label();
				}
				DrawCall::Texture(texture_id, range) => {
					buffer.begin_label(
						format!("Draw:UI/Image {}", texture_id),
						debug::LABEL_COLOR_DRAW,
					);
					buffer.set_dynamic_scissors(vec![**clips.last().unwrap()]);
					self.image.bind_pipeline(buffer);
					self.image.bind_texture(buffer, texture_id);
					self.frame_meshes[frame].bind_buffers(buffer);
					buffer.draw(range.end - range.start, range.start, 1, 0, 0);
					buffer.end_label();
				}
			}
		}
		buffer.end_label();
		Ok(())
	}
}

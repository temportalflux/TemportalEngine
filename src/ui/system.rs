use crate::{
	asset,
	graphics::{self, command, flags, font::Font, pipeline, utility::Scissor, Drawable, Texture},
	math::{vector, Matrix, Vector},
	ui::*,
	utility::{self, VoidResult},
	EngineSystem,
};
use raui::renderer::tesselate::prelude::*;
use std::{collections::HashMap, sync};

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

/// Handles the rendering of the UI widgets to the screen.
/// Also updates and processes the UI widgets via the ECS system.
pub struct System {
	draw_calls: Vec<DrawCall>,

	pending_gpu_signals: Vec<sync::Arc<command::Semaphore>>,
	atlas_mapping: HashMap<String, (String, Rect)>,
	image_sizes: HashMap<String, Vec2>,

	text_widgets: Vec<HashMap<WidgetId, text::WidgetData>>,
	text: text::DataPipeline,
	image: image::DataPipeline,
	colored_area: Drawable,
	frame_meshes: Vec<Mesh>,

	resolution: Vector<u32, 2>,
	application: Application,
}

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

		let chain_read = render_chain.read().unwrap();
		Ok(Self {
			application,
			resolution: Vector::default(),
			frame_meshes: Vec::new(),
			colored_area: Drawable::default(),
			image: image::DataPipeline::new(&chain_read)?,
			text: text::DataPipeline::new(&chain_read)?,
			text_widgets: Vec::new(),
			atlas_mapping: HashMap::new(),
			image_sizes: HashMap::new(),
			pending_gpu_signals: Vec::new(),
			draw_calls: Vec::new(),
		})
	}

	pub fn with_tree(mut self, tree: WidgetNode) -> Self {
		self.apply_tree(tree);
		self
	}

	/// Set the ui widget tree to update and render.
	pub fn apply_tree(&mut self, tree: WidgetNode) {
		self.application.apply(tree);
	}

	fn mapping(&self) -> CoordsMapping {
		CoordsMapping::new(Rect {
			left: 0.0,
			right: self.resolution.x() as f32,
			top: 0.0,
			bottom: self.resolution.y() as f32,
		})
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
		let library = crate::asset::Library::get().read().unwrap();
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
			"Adding font '{}' with width-edge {}",
			id.short_id(),
			asset.width_edge()
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
		let texture = asset::Loader::load_sync(&id)?
			.downcast::<Texture>()
			.unwrap();
		log::info!(target: LOG, "Adding texture '{}'", id.short_id());
		self.image_sizes.insert(
			id.name(),
			Vec2::from(*texture.size().try_into::<f32>().unwrap().data()),
		);
		self.image.add_pending(id, texture)?;
		Ok(())
	}

	pub fn attach_system(
		self,
		engine: &mut crate::Engine,
		render_chain: &sync::Arc<sync::RwLock<graphics::RenderChain>>,
	) -> Result<sync::Arc<sync::RwLock<Self>>, utility::AnyError> {
		let system = sync::Arc::new(sync::RwLock::new(self));
		engine.add_system(&system);
		render_chain
			.write()
			.unwrap()
			.add_render_chain_element(&system)?;
		Ok(system)
	}
}

impl EngineSystem for System {
	#[profiling::function]
	fn update(&mut self, _: std::time::Duration) {
		let mapping = self.mapping();
		self.application.process();
		let _res = self.application.layout(&mapping, &mut DefaultLayoutEngine);
	}
}

impl graphics::RenderChainElement for System {
	#[profiling::function]
	fn initialize_with(
		&mut self,
		render_chain: &mut graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		log::info!(target: LOG, "Initializing render chain element");
		self.colored_area.create_shaders(&render_chain)?;
		self.image.create_shaders(&render_chain)?;
		self.text.create_shaders(&render_chain)?;
		self.pending_gpu_signals
			.append(&mut self.text.create_pending_font_atlases(&render_chain)?);
		for _ in 0..render_chain.frame_count() {
			self.text_widgets.push(HashMap::new());
			self.frame_meshes
				.push(Mesh::new(&render_chain.allocator(), 10)?);
		}
		Ok(self.take_gpu_signals())
	}

	#[profiling::function]
	fn destroy_render_chain(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		log::info!(target: LOG, "Destroying render chain");
		self.colored_area.destroy_pipeline(render_chain)?;
		self.image.destroy_pipeline(render_chain)?;
		self.text.destroy_render_chain(render_chain)?;
		Ok(())
	}

	#[profiling::function]
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: graphics::structs::Extent2D,
	) -> utility::Result<()> {
		log::info!(target: LOG, "Creating render chain");
		self.resolution = vector![resolution.width, resolution.height];
		self.colored_area.create_pipeline(
			render_chain,
			None,
			pipeline::Info::default()
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(pipeline::ViewportState::from(resolution))
				.set_color_blending(
					pipeline::ColorBlendState::default()
						.add_attachment(pipeline::ColorBlendAttachment::default()),
				),
		)?;
		self.image.create_pipeline(render_chain, resolution)?;
		self.text
			.on_render_chain_constructed(render_chain, resolution)?;
		Ok(())
	}

	fn preframe_update(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
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
		resolution: &Vector<u32, 2>,
	) -> utility::Result<bool> {
		let mapping = self.mapping();
		// Drain the existing widgets
		let mut retained_text_widgets: HashMap<WidgetId, text::WidgetData> =
			self.text_widgets[frame].drain().collect();
		self.draw_calls = Vec::new();

		// Garuntee that there will always be a scissor clip available for the recording to use
		self.draw_calls.push(DrawCall::PushClip(Scissor::new(
			vector![0, 0],
			resolution.clone(),
		)));

		if let Some(tesselation) = self.tesselate(&mapping) {
			let mut mesh_gpu_signals =
				self.frame_meshes[frame].write(&tesselation, &render_chain, resolution)?;
			self.pending_gpu_signals.append(&mut mesh_gpu_signals);

			for batch in tesselation.batches {
				match batch {
					Batch::None | Batch::FontTriangles(_, _, _) => {}
					Batch::ColoredTriangles(range) => {
						self.draw_calls.push(DrawCall::Range(range));
					}
					Batch::ImageTriangles(texture_id, range) => {
						if self.image.has_image(&texture_id) {
							self.draw_calls.push(DrawCall::Texture(texture_id, range));
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
							text,
							retained_text_widgets.remove(&widget_id),
						)?;
						self.text_widgets[frame].insert(widget_id.clone(), widget_data);
						self.pending_gpu_signals.append(&mut gpu_signals);
						self.draw_calls.push(DrawCall::Text(widget_id));
					}
					Batch::ClipPush(clip) => {
						let matrix: Matrix<f32, 4, 4> = Matrix::from_column_major(&clip.matrix);
						let clip_vec = vector![clip.box_size.0, clip.box_size.1];
						let transform = |mask: Vector<f32, 2>| -> Vector<f32, 2> {
							let v4: Matrix<f32, 1, 4> = (clip_vec * mask).extend([0.0, 1.0].into()).into();
							let mat_vec = v4 * matrix;
							mat_vec
								.column_vec(0)
								.subvec::<2>(None)
						};

						let tl = transform(vector![0.0, 0.0]);
						let tr = transform(vector![1.0, 0.0]);
						let bl = transform(vector![0.0, 1.0]);
						let br = transform(vector![1.0, 1.0]);

						let min = tl.min(tr).min(br).min(bl);
						let max = tl.max(tr).max(br).max(bl);
						let size = max - min;

						self.draw_calls.push(DrawCall::PushClip(Scissor::new(
							[min.x() as i32, min.y() as i32].into(),
							[size.x() as u32, size.y() as u32].into(),
						)));
					}
					Batch::ClipPop => {
						self.draw_calls.push(DrawCall::PopClip());
					}
				}
			}
		}

		Ok(true) // the ui uses immediate mode, and therefore requires re-recording every frame
	}

	/// Record to the primary command buffer for a given frame
	#[profiling::function]
	fn record_to_buffer(&self, buffer: &mut command::Buffer, frame: usize) -> utility::Result<()> {
		// TODO: https://github.com/RAUI-labs/raui/discussions/52#discussioncomment-738219
		// Should use dynamic scissors on the pipeline command buffers
		let mut clips = Vec::new();
		for call in self.draw_calls.iter() {
			match call {
				DrawCall::PushClip(scissor) => clips.push(scissor),
				DrawCall::PopClip() => {
					clips.pop();
				}

				DrawCall::Text(widget_id) => self
					.text
					.record_to_buffer(buffer, &self.text_widgets[frame][widget_id])?,
				DrawCall::Range(range) => {
					self.colored_area.bind_pipeline(buffer);
					self.frame_meshes[frame].bind_buffers(buffer);
					buffer.draw(range.end - range.start, range.start, 1, 0, 0);
				}
				DrawCall::Texture(texture_id, range) => {
					self.image.bind_pipeline(buffer);
					self.image.bind_texture(buffer, texture_id);
					self.frame_meshes[frame].bind_buffers(buffer);
					buffer.draw(range.end - range.start, range.start, 1, 0, 0);
				}
			}
		}
		Ok(())
	}
}

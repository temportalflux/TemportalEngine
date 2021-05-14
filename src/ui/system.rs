use crate::{
	asset,
	graphics::{self, command, font::Font},
	math::Vector,
	ui::system_text::*,
	utility::{self, VoidResult},
	EngineSystem,
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

pub struct System {
	pending_gpu_signals: Vec<sync::Arc<command::Semaphore>>,
	atlas_mapping: HashMap<String, (String, Rect)>,
	image_sizes: HashMap<String, Vec2>,
	text: TextData,
	resolution: Vector<u32, 2>,
	application: Application,
}

impl System {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		let mut application = Application::new();
		application.setup(widget::setup);
		Ok(Self {
			application,
			resolution: Vector::default(),
			text: TextData::new(&render_chain)?,
			atlas_mapping: HashMap::new(),
			image_sizes: HashMap::new(),
			pending_gpu_signals: Vec::new(),
		})
	}

	pub fn apply_tree(&mut self, tree: WidgetNode) {
		self.application.apply(tree);
	}

	pub fn set_resolution(&mut self, resolution: Vector<u32, 2>) {
		self.resolution = resolution;
	}

	pub fn mapping(&self) -> CoordsMapping {
		CoordsMapping::new(Rect {
			left: 0.0,
			right: self.resolution.x() as f32,
			top: 0.0,
			bottom: self.resolution.y() as f32,
		})
	}

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
}

impl System {
	pub fn add_text_shader(&mut self, id: &asset::Id) -> VoidResult {
		self.text.add_shader(id)
	}

	pub fn add_font(&mut self, font_asset_id: &asset::Id) -> VoidResult {
		self.text.add_pending(
			font_asset_id.file_name(),
			asset::Loader::load_sync(&font_asset_id)?
				.downcast::<Font>()
				.unwrap(),
		);
		Ok(())
	}
}

impl EngineSystem for System {
	#[profiling::function]
	fn update(&mut self, _: std::time::Duration) {
		let mapping = self.mapping();

		self.application.process();
		let _res = self.application.layout(&mapping, &mut DefaultLayoutEngine);

		if let Some(tesselation) = self.tesselate(&mapping) {
			self.write_mesh(&tesselation);

			for batch in tesselation.batches {
				match batch {
					Batch::None | Batch::FontTriangles(_, _, _) => {}
					Batch::ColoredTriangles(_range) => {
						// TODO: range is the first to last values of `tesselation.indices` to draw for this batch
					}
					Batch::ImageTriangles(_texture_id, _range) => {
						// TODO: draw the vertices for range with the texture for texture_id bound
					}
					Batch::ExternalText(_widget_id, text) => {
						// TODO: Handle `text.matrix` https://github.com/RAUI-labs/raui/discussions/52#discussioncomment-738219
						//log::debug!("{:?}", text);
					}
					// TODO: https://github.com/RAUI-labs/raui/discussions/52#discussioncomment-738219
					Batch::ClipPush(_clip) => {}
					Batch::ClipPop => {}
				}
			}
		}
	}
}

impl System {
	#[profiling::function]
	fn write_mesh(&mut self, tesselation: &Tesselation) {
		let _vertices = tesselation.vertices.as_interleaved().unwrap();
		let _indices = &tesselation.indices;
		// TODO: write to buffers for images
	}
}

impl graphics::RenderChainElement for System {
	#[profiling::function]
	fn initialize_with(
		&mut self,
		render_chain: &mut graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		self.text.create_shaders(&render_chain)?;
		self.pending_gpu_signals
			.append(&mut self.text.create_pending_font_atlases(&render_chain)?);
		Ok(self.take_gpu_signals())
	}

	#[profiling::function]
	fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		Ok(())
	}

	#[profiling::function]
	fn on_render_chain_constructed(
		&mut self,
		_render_chain: &graphics::RenderChain,
		_resolution: graphics::structs::Extent2D,
	) -> utility::Result<()> {
		Ok(())
	}

	fn preframe_update(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		self.pending_gpu_signals
			.append(&mut self.text.create_pending_font_atlases(&render_chain)?);
		Ok(())
	}

	fn take_gpu_signals(&mut self) -> Vec<sync::Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}
}

impl graphics::CommandRecorder for System {
	/// Update the data (like uniforms) for a given frame -
	/// Or in the case of the UI Render, record changes to the secondary command buffer.
	#[profiling::function]
	fn prerecord_update(
		&mut self,
		_buffer: &command::Buffer,
		_frame: usize,
		_resolution: &Vector<u32, 2>,
	) -> utility::Result<bool> {
		Ok(true) // the ui uses immediate mode, and therefore requires re-recording every frame
	}

	/// Record to the primary command buffer for a given frame
	#[profiling::function]
	fn record_to_buffer(
		&self,
		_buffer: &mut command::Buffer,
		_frame: usize,
	) -> utility::Result<()> {
		Ok(())
	}
}

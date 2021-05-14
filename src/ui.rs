use crate::{
	graphics::{self, command},
	math::Vector,
	utility, EngineSystem,
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

pub struct System {
	pending_gpu_signals: Vec<sync::Arc<command::Semaphore>>,

	atlas_mapping: HashMap<String, (String, Rect)>,
	image_sizes: HashMap<String, Vec2>,

	resolution: Vector<u32, 2>,
	application: Application,
}

impl System {
	pub fn new() -> Self {
		let mut application = Application::new();
		application.setup(widget::setup);
		Self {
			application,
			resolution: Vector::default(),
			atlas_mapping: HashMap::new(),
			image_sizes: HashMap::new(),
			pending_gpu_signals: Vec::new(),
		}
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

impl EngineSystem for System {
	fn update(&mut self, _: std::time::Duration) {
		let mapping = self.mapping();

		self.application.process();
		let _res = self
			.application
			.layout(&mapping, &mut DefaultLayoutEngine);
			
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
						log::debug!("{:?}", text);

					}
					// TODO: shrug
					Batch::ClipPush(_clip) => {}
					Batch::ClipPop => {}
				}
			}

		}
	}
}

impl System {

	fn write_mesh(&mut self, tesselation: &Tesselation) {
		let _vertices = tesselation.vertices.as_interleaved().unwrap();
		let _indices = &tesselation.indices;
		// TODO: write to buffers for images
	}

}

impl graphics::RenderChainElement for System {
	fn initialize_with(
		&mut self,
		_render_chain: &mut graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		Ok(Vec::new())
	}

	fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		Ok(())
	}

	fn on_render_chain_constructed(
		&mut self,
		_render_chain: &graphics::RenderChain,
		_resolution: graphics::structs::Extent2D,
	) -> utility::Result<()> {
		Ok(())
	}
	
	fn take_gpu_signals(&mut self) -> Vec<sync::Arc<command::Semaphore>> {
		self.pending_gpu_signals.drain(..).collect()
	}
}

impl graphics::CommandRecorder for System {
	/// Update the data (like uniforms) for a given frame -
	/// Or in the case of the UI Render, record changes to the secondary command buffer.
	fn prerecord_update(
		&mut self,
		_buffer: &command::Buffer,
		_frame: usize,
		_resolution: &Vector<u32, 2>,
	) -> utility::Result<bool> {
		Ok(true) // the ui uses immediate mode, and therefore requires re-recording every frame
	}

	/// Record to the primary command buffer for a given frame
	fn record_to_buffer(
		&self,
		_buffer: &mut command::Buffer,
		_frame: usize,
	) -> utility::Result<()> {
		Ok(())
	}
}

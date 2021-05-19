use crate::{
	asset,
	graphics::{self, command, font::Font, utility::Scissor},
	math::{vector, Matrix, Vector},
	ui::*,
	utility::{self, VoidResult},
	EngineSystem,
};
use raui::renderer::tesselate::prelude::*;
use std::{collections::HashMap, sync};

pub struct System {
	draw_calls: Vec<DrawCall>,

	pending_gpu_signals: Vec<sync::Arc<command::Semaphore>>,
	atlas_mapping: HashMap<String, (String, Rect)>,
	image_sizes: HashMap<String, Vec2>,

	text_widgets: Vec<HashMap<WidgetId, text::WidgetData>>,
	text: text::DataPipeline,

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
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		let mut application = Application::new();
		application.setup(widget::setup);
		Ok(Self {
			application,
			resolution: Vector::default(),
			text: text::DataPipeline::new(&render_chain)?,
			text_widgets: Vec::new(),
			atlas_mapping: HashMap::new(),
			image_sizes: HashMap::new(),
			pending_gpu_signals: Vec::new(),
			draw_calls: Vec::new(),
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
	}
}

impl System {
	#[profiling::function]
	fn write_mesh(&mut self, tesselation: &Tesselation) {
		let _vertices = tesselation.vertices.as_interleaved().unwrap();
		let _indices = &tesselation.indices;
		// TODO: write to buffers for images
		log::debug!("{:?}", tesselation.vertices);
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
		self.text_widgets = (0..render_chain.frame_count())
			.map(|_| HashMap::new())
			.collect();
		Ok(self.take_gpu_signals())
	}

	#[profiling::function]
	fn destroy_render_chain(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		self.text.destroy_render_chain(render_chain)?;
		Ok(())
	}

	#[profiling::function]
	fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: graphics::structs::Extent2D,
	) -> utility::Result<()> {
		self.text
			.on_render_chain_constructed(render_chain, resolution)?;
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
			self.write_mesh(&tesselation);

			for batch in tesselation.batches {
				match batch {
					Batch::None | Batch::FontTriangles(_, _, _) => {}
					Batch::ColoredTriangles(range) => {
						self.draw_calls.push(DrawCall::Range(range));
					}
					Batch::ImageTriangles(texture_id, range) => {
						self.draw_calls.push(DrawCall::Texture(texture_id, range));
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
							(matrix * (clip_vec * mask).extend([0.0, 1.0].into()).into())
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
				DrawCall::Range(_range) => {}
				DrawCall::Texture(_texture_id, _range) => {}
			}
		}
		Ok(())
	}
}

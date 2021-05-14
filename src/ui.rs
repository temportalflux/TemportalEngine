use crate::{
	asset,
	graphics::{
		self, command, flags,
		font::{Font, Glyph}, buffer,
		image_view, sampler,
	},
	math::Vector,
	task,
	utility::{self, VoidResult},
	EngineSystem,
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

type FontId = String;
type UnicodeId = u32;
type FontGlyphMap = HashMap<UnicodeId, Glyph>;
struct PendingFontAtlas {
	size: Vector<usize, 2>,
	binary: Vec<u8>,
	format: flags::Format,
	glyph_map: FontGlyphMap,
}
struct LoadedFont {
	glyph_map: FontGlyphMap,
	view: sync::Arc<image_view::View>,
	//vertex_buffer: sync::Arc<buffer::Buffer>,
	//index_buffer: sync::Arc<buffer::Buffer>,
}

struct TextData {
	pending_font_atlases: HashMap<FontId, PendingFontAtlas>,
	fonts: HashMap<FontId, LoadedFont>,

	sampler: sync::Arc<sampler::Sampler>,
}

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
			text: TextData {
				sampler: sync::Arc::new(
					graphics::sampler::Sampler::builder()
						.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
						.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
						.build(&render_chain.logical())?,
				),
				fonts: HashMap::new(),
				pending_font_atlases: HashMap::new(),
			},
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
	pub fn add_font(&mut self, font_asset_id: &asset::Id) -> VoidResult {
		let font = asset::Loader::load_sync(&font_asset_id)?
			.downcast::<Font>()
			.unwrap();
		self.text.pending_font_atlases.insert(
			font_asset_id.file_name(),
			PendingFontAtlas {
				size: *font.size(),
				binary: font.binary().iter().flatten().map(|alpha| *alpha).collect(),
				format: flags::Format::R8_SRGB,
				glyph_map: font
					.glyphs()
					.iter()
					.map(|glyph| (glyph.unicode, glyph.clone()))
					.collect(),
			},
		);
		Ok(())
	}

	fn create_pending_font_atlases(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		if self.text.pending_font_atlases.is_empty() {
			return Ok(());
		}
		for (id, pending) in self.text.pending_font_atlases.drain() {
			let (view, mut signals) = Self::create_font_atlas(render_chain, &pending)?;
			self.pending_gpu_signals.append(&mut signals);
			self.text.fonts.insert(
				id,
				LoadedFont {
					glyph_map: pending.glyph_map,
					view,
				},
			);
		}
		Ok(())
	}

	fn create_font_atlas(
		render_chain: &graphics::RenderChain,
		pending: &PendingFontAtlas,
	) -> utility::Result<(
		sync::Arc<image_view::View>,
		Vec<sync::Arc<command::Semaphore>>,
	)> {
		use graphics::{alloc, image, structs::subresource, TaskCopyImageToGpu};
		let mut signals = Vec::new();

		let image_size = pending.size.subvec::<3>(None).with_z(1);
		let image = sync::Arc::new(
			image::Image::builder()
				.with_alloc(
					alloc::Info::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_format(pending.format)
				.with_size(image_size)
				.with_usage(flags::ImageUsage::TRANSFER_DST)
				.with_usage(flags::ImageUsage::SAMPLED)
				.build(&render_chain.allocator())?,
		);

		let copy_task = TaskCopyImageToGpu::new(&render_chain)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&pending.binary[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.end()?;
		signals.push(copy_task.gpu_signal_on_complete());
		copy_task.send_to(task::sender());

		let view = sync::Arc::new(
			image_view::View::builder()
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_format(pending.format)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&render_chain.logical())?,
		);

		Ok((view, signals))
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
						log::debug!("{:?}", text);
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
		self.create_pending_font_atlases(&render_chain)?;
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
		self.create_pending_font_atlases(&render_chain)?;
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

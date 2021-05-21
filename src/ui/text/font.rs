use crate::{
	graphics::{
		self, command, flags,
		font::{Font, Glyph},
		image_view,
	},
	math::Vector,
	task, utility,
};
use std::{collections::HashMap, sync};

pub type Id = String;
type UnicodeId = u32;
type FontGlyphMap = HashMap<UnicodeId, Glyph>;

pub type FGetWidthEdge = dyn Fn(f32) -> Vector<f32, 2> + Send + Sync;
pub type BoxedGetWidthEdge = sync::Arc<FGetWidthEdge>;

pub struct PendingAtlas {
	size: Vector<usize, 2>,
	binary: Vec<u8>,
	format: flags::Format,
	glyph_map: FontGlyphMap,
	line_height: f32,
	get_width_edge: BoxedGetWidthEdge,
}

pub struct Loaded {
	size: Vector<usize, 2>,
	line_height: f32,
	glyph_map: FontGlyphMap,
	view: sync::Arc<image_view::View>,
	get_width_edge: BoxedGetWidthEdge,
}

impl From<Box<Font>> for PendingAtlas {
	fn from(font: Box<Font>) -> Self {
		Self {
			size: *font.size(),
			binary: font.binary().iter().flatten().map(|alpha| *alpha).collect(),
			format: flags::Format::R8_SRGB,
			glyph_map: font
				.glyphs()
				.iter()
				.map(|glyph| (glyph.unicode, glyph.clone()))
				.collect(),
			line_height: *font.line_height(),
			get_width_edge: sync::Arc::new(|_| Vector::new([0.6, 0.2])),
		}
	}
}

impl PendingAtlas {
	pub fn with_width_edge(mut self, get_width_edge: BoxedGetWidthEdge) -> Self {
		self.get_width_edge = get_width_edge;
		self
	}

	pub fn load(
		self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<(Loaded, Vec<sync::Arc<command::Semaphore>>)> {
		use graphics::{alloc, image, structs::subresource, TaskGpuCopy};
		let mut signals = Vec::new();

		let image_size = self.size.subvec::<3>(None).with_z(1);
		let image = sync::Arc::new(
			image::Image::builder()
				.with_alloc(
					alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_format(self.format)
				.with_size(image_size)
				.with_usage(flags::ImageUsage::TRANSFER_DST)
				.with_usage(flags::ImageUsage::SAMPLED)
				.build(&render_chain.allocator())?,
		);

		let copy_task = TaskGpuCopy::new(&render_chain)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&self.binary[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.end()?;
		signals.push(copy_task.gpu_signal_on_complete());
		copy_task.send_to(task::sender());

		let view = sync::Arc::new(
			image_view::View::builder()
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_format(self.format)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&render_chain.logical())?,
		);

		Ok((
			Loaded {
				view,
				size: self.size,
				glyph_map: self.glyph_map,
				line_height: self.line_height,
				get_width_edge: self.get_width_edge,
			},
			signals,
		))
	}
}

impl Loaded {
	pub fn view(&self) -> &sync::Arc<image_view::View> {
		&self.view
	}

	pub fn get(&self, i: char) -> Option<&Glyph> {
		self.glyph_map.get(&(i as u32))
	}

	pub fn size(&self) -> Vector<f32, 2> {
		self.size.try_into::<f32>().unwrap()
	}

	pub fn line_height(&self) -> &f32 {
		&self.line_height
	}

	pub fn get_width_edge(&self, font_size: f32) -> Vector<f32, 2> {
		self.get_width_edge.as_ref()(font_size)
	}
}

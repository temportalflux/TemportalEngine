use crate::{
	graphics::{
		self, command, flags,
		font::{Font, Glyph},
		image_view, structs,
		utility::NameableBuilder,
	},
	math::nalgebra::Vector2,
	task, utility,
};
use std::{collections::HashMap, sync};

pub type Id = String;
type UnicodeId = u32;
type FontGlyphMap = HashMap<UnicodeId, Glyph>;

pub struct PendingAtlas {
	id: String,
	size: Vector2<usize>,
	binary: Vec<u8>,
	format: flags::format::Format,
	glyph_map: FontGlyphMap,
	line_height: f32,
	width_edge: Vector2<f32>,
}

pub struct Loaded {
	size: Vector2<usize>,
	line_height: f32,
	glyph_map: FontGlyphMap,
	view: sync::Arc<image_view::View>,
	width_edge: Vector2<f32>,
}

impl PendingAtlas {
	pub fn from(id: String, font: Box<Font>) -> Self {
		Self {
			id,
			size: *font.size(),
			binary: font.binary().iter().flatten().map(|alpha| *alpha).collect(),
			format: flags::format::SRGB_8BIT_R,
			glyph_map: font
				.glyphs()
				.iter()
				.map(|glyph| (glyph.unicode, glyph.clone()))
				.collect(),
			line_height: *font.line_height(),
			width_edge: *font.width_edge(),
			//width_edge: [0.8, 0.05].into(),
		}
	}
}

impl PendingAtlas {
	pub fn load(
		self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<(Loaded, Vec<sync::Arc<command::Semaphore>>)> {
		use graphics::{
			alloc, image,
			structs::subresource,
			utility::{BuildFromAllocator, BuildFromDevice},
			TaskGpuCopy,
		};
		let mut signals = Vec::new();

		let image = sync::Arc::new(
			image::Image::builder()
				.with_name(format!("UI.FontAtlas:{}.Image", self.id))
				.with_alloc(
					alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_format(self.format)
				.with_size(structs::Extent3D {
					width: self.size.x as u32,
					height: self.size.y as u32,
					depth: 1,
				})
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
				.with_name(format!("UI.FontAtlas:{}.Image.View", self.id))
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&render_chain.logical())?,
		);

		Ok((
			Loaded {
				view,
				size: self.size,
				glyph_map: self.glyph_map,
				line_height: self.line_height,
				width_edge: self.width_edge,
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

	pub fn size(&self) -> Vector2<f32> {
		[self.size.x as f32, self.size.y as f32].into()
	}

	pub fn line_height(&self) -> &f32 {
		&self.line_height
	}

	pub fn get_width_edge(&self) -> &Vector2<f32> {
		&self.width_edge
	}
}

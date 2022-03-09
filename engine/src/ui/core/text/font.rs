use crossbeam_channel::Sender;

use crate::{
	graphics::{
		self, command, flags,
		font::{Font, Glyph},
		image_view, structs,
		utility::{NameableBuilder, NamedObject},
		GpuOpContext,
	},
	math::nalgebra::Vector2,
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
	name: String,
	size: Vector2<usize>,
	line_height: f32,
	glyph_map: FontGlyphMap,
	view: sync::Arc<image_view::View>,
	width_edge: Vector2<f32>,
}

impl PendingAtlas {
	pub fn from(id: String, font: Box<Font>) -> Self {
		use flags::format::prelude::*;

		let binary = {
			let font_binary = font.binary().iter().flatten().collect::<Vec<_>>();
			let mut binary = Vec::with_capacity(font_binary.len() * 4);
			for alpha in font_binary.into_iter() {
				binary.push(*alpha); // r
				binary.push(0); // g
				binary.push(0); // b
				binary.push(*alpha);
			}
			binary
		};

		Self {
			id,
			size: *font.size(),
			binary: binary,
			// Only need the red channel, but "Intel Iris Pro Graphics" doesnt suppport R8_SRGB (it does support RGBA 8-bit SRGB)
			format: flags::format::format(&[R, G, B, A], Bit8, SRGB),
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
		context: &impl GpuOpContext,
		signal_sender: &Sender<sync::Arc<command::Semaphore>>,
	) -> anyhow::Result<Loaded> {
		use graphics::{
			alloc, image,
			structs::subresource,
			utility::{BuildFromAllocator, BuildFromDevice},
			GpuOperationBuilder,
		};

		let atlas_name = format!("UI.FontAtlas:{}", self.id);

		let image = sync::Arc::new(
			image::Image::builder()
				.with_name(format!("{}.Image", atlas_name))
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
				.build(&context.object_allocator()?)?,
		);

		GpuOperationBuilder::new(image.wrap_name(|v| format!("Create({})", v)), context)?
			.begin()?
			.format_image_for_write(&image)
			.stage(&self.binary[..])?
			.copy_stage_to_image(&image)
			.format_image_for_read(&image)
			.send_signal_to(signal_sender)?
			.end()?;

		let view = sync::Arc::new(
			image_view::View::builder()
				.with_name(format!("{}.Image.View", atlas_name))
				.for_image(image.clone())
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(subresource::Range::default().with_aspect(flags::ImageAspect::COLOR))
				.build(&context.logical_device()?)?,
		);

		Ok(Loaded {
			name: atlas_name,
			view,
			size: self.size,
			glyph_map: self.glyph_map,
			line_height: self.line_height,
			width_edge: self.width_edge,
		})
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

	pub fn name(&self) -> &String {
		&self.name
	}
}

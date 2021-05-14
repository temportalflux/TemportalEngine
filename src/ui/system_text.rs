use crate::{
	graphics::{
		self, buffer, command, flags,
		font::{Font, Glyph},
		image_view, sampler,
	},
	math::Vector,
	task, utility,
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

pub type FontId = String;
pub type UnicodeId = u32;
pub type FontGlyphMap = HashMap<UnicodeId, Glyph>;
pub struct PendingFontAtlas {
	size: Vector<usize, 2>,
	binary: Vec<u8>,
	format: flags::Format,
	glyph_map: FontGlyphMap,
}
pub struct LoadedFont {
	glyph_map: FontGlyphMap,
	view: sync::Arc<image_view::View>,
	//vertex_buffer: sync::Arc<buffer::Buffer>,
	//index_buffer: sync::Arc<buffer::Buffer>,
}

pub struct TextData {
	pending_font_atlases: HashMap<FontId, PendingFontAtlas>,
	fonts: HashMap<FontId, LoadedFont>,

	sampler: sync::Arc<sampler::Sampler>,
}

impl TextData {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		Ok(Self {
			sampler: sync::Arc::new(
				graphics::sampler::Sampler::builder()
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
					.build(&render_chain.logical())?,
			),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
		})
	}

	pub fn add_pending(&mut self, id: String, font: Box<Font>) {
		self.pending_font_atlases.insert(
			id,
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
	}

	pub fn create_pending_font_atlases(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let mut pending_gpu_signals = Vec::new();
		if !self.pending_font_atlases.is_empty() {
			for (id, pending) in self.pending_font_atlases.drain() {
				let (view, mut signals) = Self::create_font_atlas(render_chain, &pending)?;
				pending_gpu_signals.append(&mut signals);
				self.fonts.insert(
					id,
					LoadedFont {
						glyph_map: pending.glyph_map,
						view,
					},
				);
			}
		}
		Ok(pending_gpu_signals)
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

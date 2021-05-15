use crate::{
	asset,
	graphics::{self, command, flags, font::Font, sampler, shader},
	math::{vector, Matrix, Vector},
	ui::{
		self,
		text::{font, Vertex, WidgetData},
	},
	utility::{self, VoidResult},
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

pub struct DataPipeline {
	pending_font_atlases: HashMap<font::Id, font::PendingAtlas>,
	fonts: HashMap<font::Id, font::Loaded>,

	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
	sampler: sync::Arc<sampler::Sampler>,
}

impl DataPipeline {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		Ok(Self {
			sampler: sync::Arc::new(
				graphics::sampler::Sampler::builder()
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
					.build(&render_chain.logical())?,
			),
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();
		self.pending_shaders
			.insert(shader.kind(), shader.contents().clone());
		Ok(())
	}

	pub fn add_pending(&mut self, id: String, font: Box<Font>) {
		self.pending_font_atlases.insert(id, font.into());
	}

	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		for (kind, binary) in self.pending_shaders.drain() {
			self.shaders.insert(
				kind,
				sync::Arc::new(shader::Module::create(
					render_chain.logical().clone(),
					shader::Info {
						kind: kind,
						entry_point: String::from("main"),
						bytes: binary,
					},
				)?),
			);
		}
		Ok(())
	}

	pub fn create_pending_font_atlases(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let mut pending_gpu_signals = Vec::new();
		if !self.pending_font_atlases.is_empty() {
			for (id, pending) in self.pending_font_atlases.drain() {
				let (loaded, mut signals) = pending.load(render_chain)?;
				pending_gpu_signals.append(&mut signals);
				self.fonts.insert(id, loaded);
			}
		}
		Ok(pending_gpu_signals)
	}

	pub fn create_item(
		&self,
		text: BatchExternalText,
		resolution: &Vector<u32, 2>,
	) -> Result<WidgetData, ui::Error> {
		let font = self
			.fonts
			.get(&text.font)
			.ok_or(ui::Error::InvalidFont(text.font.clone()))?;
		let (vertices, indices) = self.build_buffer_data(&text, font, resolution);
		Ok(WidgetData {})
	}

	pub fn update_item(
		&self,
		widget: WidgetData,
		text: BatchExternalText,
		resolution: &Vector<u32, 2>,
	) -> WidgetData {
		widget
	}

	fn build_buffer_data(
		&self,
		text: &BatchExternalText,
		font: &font::Loaded,
		resolution: &Vector<u32, 2>,
	) -> (Vec<Vertex>, Vec<u16>) {
		let resolution: Vector<f32, 2> = vector![resolution.x() as f32, resolution.y() as f32];
		// DPI is always 1.0 because winit handles the scale factor https://docs.rs/winit/0.24.0/winit/dpi/index.html
		let dpi = 1_f32;

		let matrix: Matrix<f32, 4, 4> = Matrix::from_column_major(&text.matrix);
		let base_pos: Matrix<f32, 1, 4> = Matrix::new([[0.0], [0.0], [0.0], [1.0]]);
		let screen_pos_pixels = (base_pos * matrix).column_vec(0).subvec::<2>(None);

		let unknown_glyph = font.get('?').unwrap();
		let mut vertices = Vec::with_capacity(text.text.len() * 4);
		let mut indices = Vec::with_capacity(text.text.len() * 6);

		let mut push_vertex = |vertex: Vertex| {
			let index = vertices.len();
			vertices.push(vertex);
			index as u16
		};

		// The position of the cursor on screen in pixels.
		// The cursor starts at the screen_pos but with an extra bump down
		// equivalent to the distance between the baseline and the top of the line.
		let line_height = font.line_height() * text.size * dpi;
		let mut cursor_pos = screen_pos_pixels + vector![0.0, line_height];

		for unicode in text.text.chars() {
			let color = vector![text.color.0, text.color.1, text.color.2, text.color.3];

			let glyph = font.get(unicode).unwrap_or(unknown_glyph);
			let glyph_metrics = glyph.metrics * text.size * dpi;
			let bearing = vector![glyph_metrics.bearing.x(), -glyph_metrics.bearing.y()];
			let atlas_pos = Vector::new([glyph.atlas_pos.x() as f32, glyph.atlas_pos.y() as f32]);

			let make_glyph_vert = |mask: Vector<f32, 2>| -> Vertex {
				let pos = (cursor_pos + bearing + glyph_metrics.size * mask) / resolution;
				let tex_coord = ((atlas_pos * mask) / font.size()).subvec::<4>(None);
				Vertex {
					pos_and_width_edge: vector![pos.x(), pos.y(), 0.5, 0.1],
					tex_coord,
					color,
				}
			};

			let tl = push_vertex(make_glyph_vert(vector![0.0, 0.0]));
			let tr = push_vertex(make_glyph_vert(vector![1.0, 0.0]));
			let bl = push_vertex(make_glyph_vert(vector![0.0, 1.0]));
			let br = push_vertex(make_glyph_vert(vector![1.0, 1.0]));
			indices.push(tl);
			indices.push(tr);
			indices.push(br);
			indices.push(br);
			indices.push(bl);
			indices.push(tl);

			*cursor_pos.x_mut() += glyph_metrics.advance;
		}
		(vertices, indices)
	}

}

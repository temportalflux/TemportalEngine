use crate::{
	graphics::{self, buffer, command, flags},
	math::{vector, Matrix, Vector},
	task,
	ui::text::{font, Vertex},
	utility,
};
pub use raui::prelude::*;
use std::sync;

/// Data about rendering a specific piece of text
pub struct WidgetData {
	content: String,
	font_id: font::Id,
	font_size: f32,
	color: Vector<f32, 4>,
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
}

impl WidgetData {
	pub fn new(
		text: &BatchExternalText,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Self> {
		Ok(Self {
			content: text.text.clone(),
			font_id: text.font.clone(),
			font_size: text.size,
			color: vector![text.color.0, text.color.1, text.color.2, text.color.3],
			vertex_buffer: Self::allocate_buffer(
				flags::BufferUsage::VERTEX_BUFFER,
				&text.text,
				render_chain,
			)?,
			index_buffer: Self::allocate_buffer(
				flags::BufferUsage::INDEX_BUFFER,
				&text.text,
				render_chain,
			)?,
			index_count: 0,
		})
	}

	pub fn font_id(&self) -> &font::Id {
		&self.font_id
	}

	pub fn index_count(&self) -> &usize {
		&self.index_count
	}

	fn vertex_buffer_size_for(content: &String) -> usize {
		std::mem::size_of::<Vertex>() * content.len() * 4
	}

	fn index_buffer_size_for(content: &String) -> usize {
		std::mem::size_of::<u32>() * content.len() * 6
	}

	fn allocate_buffer(
		usage: flags::BufferUsage,
		content: &String,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<sync::Arc<buffer::Buffer>> {
		Ok(sync::Arc::new(
			graphics::buffer::Buffer::builder()
				.with_usage(usage)
				.with_usage(flags::BufferUsage::TRANSFER_DST)
				.with_size(match usage {
					flags::BufferUsage::VERTEX_BUFFER => Self::vertex_buffer_size_for(&content),
					flags::BufferUsage::INDEX_BUFFER => Self::index_buffer_size_for(&content),
					_ => 0,
				})
				.with_alloc(
					graphics::alloc::Info::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_sharing(flags::SharingMode::EXCLUSIVE)
				.build(&render_chain.allocator())?,
		))
	}

	pub fn write_buffer_data(
		&mut self,
		text: &BatchExternalText,
		font: &font::Loaded,
		render_chain: &graphics::RenderChain,
		resolution: &Vector<u32, 2>,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		use graphics::alloc::Object;

		// Update the buffer objects if we need more space than is currently allocated
		if Self::vertex_buffer_size_for(&text.text) > self.vertex_buffer.size() {
			if !sync::Arc::get_mut(&mut self.vertex_buffer).unwrap().resize(
				&render_chain.allocator(),
				Self::vertex_buffer_size_for(&text.text),
			) {
				self.vertex_buffer = Self::allocate_buffer(
					flags::BufferUsage::VERTEX_BUFFER,
					&text.text,
					render_chain,
				)?;
			}
			if !sync::Arc::get_mut(&mut self.index_buffer).unwrap().resize(
				&render_chain.allocator(),
				Self::index_buffer_size_for(&text.text),
			) {
				self.index_buffer = Self::allocate_buffer(
					flags::BufferUsage::INDEX_BUFFER,
					&text.text,
					render_chain,
				)?;
			}
		}

		self.content = text.text.clone();
		self.font_id = text.font.clone();
		self.font_size = text.size;
		self.color = vector![text.color.0, text.color.1, text.color.2, text.color.3];

		let mut gpu_signals = Vec::with_capacity(2);
		let (vertices, indices) =
			self.build_buffer_data(Matrix::from_column_major(&text.matrix), font, resolution);
		self.index_count = indices.len();

		graphics::TaskGpuCopy::new(&render_chain)?
			.begin()?
			.stage(&vertices[..])?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		graphics::TaskGpuCopy::new(&render_chain)?
			.begin()?
			.stage(&indices[..])?
			.copy_stage_to_buffer(&self.index_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		Ok(gpu_signals)
	}

	fn build_buffer_data(
		&self,
		matrix: Matrix<f32, 4, 4>,
		font: &font::Loaded,
		resolution: &Vector<u32, 2>,
	) -> (Vec<Vertex>, Vec<u32>) {
		let resolution: Vector<f32, 2> = vector![resolution.x() as f32, resolution.y() as f32];
		// DPI is always 1.0 because winit handles the scale factor https://docs.rs/winit/0.24.0/winit/dpi/index.html
		let dpi = 1_f32;

		let base_pos: Matrix<f32, 1, 4> = Matrix::new([[0.0], [0.0], [0.0], [1.0]]);
		let screen_pos_pixels = (base_pos * matrix).column_vec(0).subvec::<2>(None);

		let unknown_glyph = font.get('?').unwrap();
		let mut vertices = Vec::with_capacity(self.content.len() * 4);
		let mut indices = Vec::with_capacity(self.content.len() * 6);

		let mut push_vertex = |vertex: Vertex| {
			let index = vertices.len();
			vertices.push(vertex);
			index as u32
		};

		// The position of the cursor on screen in pixels.
		// The cursor starts at the screen_pos but with an extra bump down
		// equivalent to the distance between the baseline and the top of the line.
		let line_height = font.line_height() * self.font_size * dpi;
		let mut cursor_pos = screen_pos_pixels + vector![0.0, line_height];

		for unicode in self.content.chars() {
			let glyph = font.get(unicode).unwrap_or(unknown_glyph);
			let glyph_metrics = glyph.metrics * self.font_size * dpi;
			let bearing = vector![glyph_metrics.bearing.x(), -glyph_metrics.bearing.y()];
			let glyph_pos_in_atlas =
				Vector::new([glyph.atlas_pos.x() as f32, glyph.atlas_pos.y() as f32]);
			let glyph_size_in_atlas =
				Vector::new([glyph.atlas_size.x() as f32, glyph.atlas_size.y() as f32]);

			let mut push_glyph_vert = |mask: Vector<f32, 2>| -> u32 {
				let mut pos = (cursor_pos + bearing + glyph_metrics.size * mask) / resolution;
				pos *= 2.0;
				pos -= 1.0;
				let tex_coord = ((glyph_pos_in_atlas + glyph_size_in_atlas * mask) / font.size())
					.subvec::<4>(None);
				push_vertex(Vertex {
					pos_and_width_edge: vector![pos.x(), pos.y(), 0.6, 0.2],
					tex_coord,
					color: self.color,
				})
			};

			if unicode != ' ' {
				let tl = push_glyph_vert(vector![0.0, 0.0]);
				let tr = push_glyph_vert(vector![1.0, 0.0]);
				let bl = push_glyph_vert(vector![0.0, 1.0]);
				let br = push_glyph_vert(vector![1.0, 1.0]);
				indices.push(tl);
				indices.push(tr);
				indices.push(br);
				indices.push(br);
				indices.push(bl);
				indices.push(tl);
			}

			*cursor_pos.x_mut() += glyph_metrics.advance;
		}
		(vertices, indices)
	}

	pub fn bind_buffers(&self, buffer: &mut command::Buffer) {
		buffer.bind_vertex_buffers(0, vec![&self.vertex_buffer], vec![0]);
		buffer.bind_index_buffer(&self.index_buffer, 0);
	}
}

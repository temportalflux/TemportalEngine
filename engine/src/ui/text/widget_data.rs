use crate::{
	graphics::{self, buffer, command, flags},
	math::nalgebra::{vector, Matrix4, Vector2, Vector4},
	task,
	ui::text::{font, Vertex},
	utility,
};
use raui::renderer::tesselate::prelude::*;
use std::sync;

/// Data about rendering a specific piece of text
pub struct WidgetData {
	content: String,
	font_id: font::Id,
	font_size: f32,
	color: Vector4<f32>,
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
}

impl WidgetData {
	pub fn new(
		id: Option<String>,
		text: &BatchExternalText,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Self> {
		// matches: "/.", "/<*>",
		// and the series "/<0>", "/<1>", "/<2>", ..., "/<n>"
		let id_regex = regex::Regex::new(r"(/\.)|(?:/<\*>)|(?:/<[0-9]*>)").unwrap();
		let short_name = id.as_ref().map(|name| id_regex.replace_all(name, ""));
		Ok(Self {
			content: text.text.clone(),
			font_id: text.font.clone(),
			font_size: text.size,
			color: vector![text.color.r, text.color.g, text.color.b, text.color.a],
			vertex_buffer: Self::allocate_buffer(
				short_name
					.as_ref()
					.map(|name| format!("UI.{}.VertexBuffer", name)),
				flags::BufferUsage::VERTEX_BUFFER,
				&text.text,
				render_chain,
			)?,
			index_buffer: Self::allocate_buffer(
				short_name
					.as_ref()
					.map(|name| format!("UI.{}.IndexBuffer", name)),
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
		name: Option<String>,
		usage: flags::BufferUsage,
		content: &String,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<sync::Arc<buffer::Buffer>> {
		Ok(sync::Arc::new(
			graphics::buffer::Buffer::builder()
				.with_optname(name)
				.with_usage(usage)
				.with_usage(flags::BufferUsage::TRANSFER_DST)
				.with_size(match usage {
					flags::BufferUsage::VERTEX_BUFFER => Self::vertex_buffer_size_for(&content),
					flags::BufferUsage::INDEX_BUFFER => Self::index_buffer_size_for(&content),
					_ => 0,
				})
				.with_index_type(match usage {
					flags::BufferUsage::INDEX_BUFFER => Some(flags::IndexType::UINT32),
					_ => None,
				})
				.with_alloc(
					graphics::alloc::Builder::default()
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
		resolution: &Vector2<f32>,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		// Update the buffer objects if we need more space than is currently allocated
		sync::Arc::get_mut(&mut self.vertex_buffer)
			.unwrap()
			.expand(Self::vertex_buffer_size_for(&text.text))?;
		sync::Arc::get_mut(&mut self.index_buffer)
			.unwrap()
			.expand(Self::index_buffer_size_for(&text.text))?;

		self.content = text.text.clone();
		self.font_id = text.font.clone();
		self.font_size = text.size;
		self.color = vector![text.color.r, text.color.g, text.color.b, text.color.a];

		let mut gpu_signals = Vec::with_capacity(2);
		let column_major = Matrix4::<f32>::from_vec_generic(
			nalgebra::Const::<4>,
			nalgebra::Const::<4>,
			text.matrix.to_vec(),
		);
		let (vertices, indices) = self.build_buffer_data(column_major, font, resolution);
		self.index_count = indices.len();

		graphics::TaskGpuCopy::new(&render_chain)?
			.begin()?
			.set_stage_target(&self.vertex_buffer)
			.stage(&vertices[..])?
			.copy_stage_to_buffer(&self.vertex_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		graphics::TaskGpuCopy::new(&render_chain)?
			.begin()?
			.set_stage_target(&self.index_buffer)
			.stage(&indices[..])?
			.copy_stage_to_buffer(&self.index_buffer)
			.end()?
			.add_signal_to(&mut gpu_signals)
			.send_to(task::sender());

		Ok(gpu_signals)
	}

	fn build_buffer_data(
		&self,
		matrix: Matrix4<f32>,
		font: &font::Loaded,
		resolution: &Vector2<f32>,
	) -> (Vec<Vertex>, Vec<u32>) {
		// DPI is always 1.0 because winit handles the scale factor https://docs.rs/winit/0.24.0/winit/dpi/index.html
		let dpi = 1_f32;

		let base_pos = Vector4::new(0.0, 0.0, 0.0, 1.0);
		let screen_pos_pixels = (matrix * base_pos).xy();

		let unknown_glyph = font.get('?').unwrap();
		let mut vertices = Vec::with_capacity(self.content.len() * 4);
		let mut indices = Vec::with_capacity(self.content.len() * 6);

		let width_edge = *font.get_width_edge();
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
			let bearing = vector![glyph_metrics.bearing.x, -glyph_metrics.bearing.y];
			let glyph_pos_in_atlas = vector![glyph.atlas_pos.x as f32, glyph.atlas_pos.y as f32];
			let glyph_size_in_atlas = vector![glyph.atlas_size.x as f32, glyph.atlas_size.y as f32];

			let mut push_glyph_vert = |mask: Vector2<f32>| -> u32 {
				let pos = (cursor_pos + bearing + glyph_metrics.size.component_mul(&mask))
					.component_div(resolution);
				let tex_coord = (glyph_pos_in_atlas + glyph_size_in_atlas.component_mul(&mask))
					.component_div(&font.size());
				push_vertex(Vertex {
					pos_and_width_edge: [
						pos.x * 2.0 - 1.0,
						pos.y * 2.0 - 1.0,
						width_edge.x,
						width_edge.y,
					]
					.into(),
					tex_coord: tex_coord.into(),
					color: self.color.into(),
				})
			};

			if !unicode.is_whitespace() {
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

			if unicode == '\n' {
				cursor_pos.y += line_height;
				cursor_pos.x = screen_pos_pixels.x;
			} else {
				cursor_pos.x += glyph_metrics.advance;
			}
		}
		(vertices, indices)
	}

	pub fn bind_buffers(&self, buffer: &mut command::Buffer) {
		buffer.bind_vertex_buffers(0, vec![&self.vertex_buffer], vec![0]);
		buffer.bind_index_buffer(&self.index_buffer, 0);
	}
}

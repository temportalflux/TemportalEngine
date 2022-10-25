use crate::channels::mpsc::Sender;

use super::{font, Vertex};
use crate::{
	graphics::{self, alloc, buffer, command, flags, utility::NamedObject, GpuOpContext},
	math::nalgebra::{vector, Matrix4, Vector2, Vector4},
	ui::raui::*,
};
use std::sync::{self, Arc};

/// Data about rendering a specific piece of text
pub struct WidgetData {
	content: String,
	font_id: font::Id,
	font_size: f32,
	color: Vector4<f32>,
	index_count: usize,
	index_buffer: sync::Arc<buffer::Buffer>,
	vertex_buffer: sync::Arc<buffer::Buffer>,
	name: String,
}

impl WidgetData {
	pub fn new(
		id: String,
		text: &BatchExternalText,
		allocator: &Arc<alloc::Allocator>,
	) -> anyhow::Result<Self> {
		// matches: "/.", "/<*>",
		// and the series "/<0>", "/<1>", "/<2>", ..., "/<n>"
		let id_regex = regex::Regex::new(r"(/\.)|(?:/<\*>)|(?:/<[0-9]*>)").unwrap();
		let short_name = id_regex.replace_all(&id, "");
		Ok(Self {
			content: text.text.clone(),
			font_id: text.font.clone(),
			font_size: text.size,
			color: vector![text.color.r, text.color.g, text.color.b, text.color.a],
			vertex_buffer: Self::allocate_buffer(
				format!("UI.{}.VertexBuffer", short_name),
				flags::BufferUsage::VERTEX_BUFFER,
				&text.text,
				allocator,
			)?,
			index_buffer: Self::allocate_buffer(
				format!("UI.{}.IndexBuffer", short_name),
				flags::BufferUsage::INDEX_BUFFER,
				&text.text,
				allocator,
			)?,
			index_count: 0,
			name: short_name.to_owned(),
		})
	}

	pub fn font_id(&self) -> &font::Id {
		&self.font_id
	}

	pub fn index_count(&self) -> &usize {
		&self.index_count
	}

	pub fn name(&self) -> &String {
		&self.name
	}

	fn vertex_buffer_size_for(content: &String) -> usize {
		std::mem::size_of::<Vertex>() * content.len() * 4
	}

	fn index_buffer_size_for(content: &String) -> usize {
		std::mem::size_of::<u32>() * content.len() * 6
	}

	fn allocate_buffer(
		name: String,
		usage: flags::BufferUsage,
		content: &String,
		allocator: &Arc<alloc::Allocator>,
	) -> anyhow::Result<sync::Arc<buffer::Buffer>> {
		use graphics::utility::{BuildFromAllocator, NameableBuilder};
		Ok(sync::Arc::new(
			graphics::buffer::Buffer::builder()
				.with_name(name)
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
				.with_location(flags::MemoryLocation::GpuOnly)
				.with_sharing(flags::SharingMode::EXCLUSIVE)
				.build(allocator)?,
		))
	}

	pub fn write_buffer_data(
		&mut self,
		text: &BatchExternalText,
		font: &font::Loaded,
		context: &impl GpuOpContext,
		resolution: &Vector2<f32>,
		signal_sender: &Sender<Arc<command::Semaphore>>,
	) -> anyhow::Result<()> {
		// Update the buffer objects if we need more space than is currently allocated
		if let Some(reallocated) = self
			.vertex_buffer
			.expand(Self::vertex_buffer_size_for(&text.text))?
		{
			self.vertex_buffer = sync::Arc::new(reallocated);
		}
		if let Some(reallocated) = self
			.index_buffer
			.expand(Self::index_buffer_size_for(&text.text))?
		{
			self.index_buffer = sync::Arc::new(reallocated);
		}

		self.content = text.text.clone();
		self.font_id = text.font.clone();
		self.font_size = text.size;
		self.color = vector![text.color.r, text.color.g, text.color.b, text.color.a];

		let column_major = Matrix4::<f32>::from_vec_generic(
			nalgebra::Const::<4>,
			nalgebra::Const::<4>,
			text.matrix.to_vec(),
		);
		let (vertices, indices) = self.build_buffer_data(column_major, font, &resolution);
		self.index_count = indices.len();

		graphics::GpuOperationBuilder::new(
			format!("Write({})", self.vertex_buffer.name()),
			context,
		)?
		.begin()?
		.stage(&vertices[..])?
		.copy_stage_to_buffer(&self.vertex_buffer)
		.send_signal_to(signal_sender)?
		.end()?;

		graphics::GpuOperationBuilder::new(
			format!("Write({})", self.index_buffer.name()),
			context,
		)?
		.begin()?
		.stage(&indices[..])?
		.copy_stage_to_buffer(&self.index_buffer)
		.send_signal_to(signal_sender)?
		.end()?;

		Ok(())
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

use crate::{
	graphics::{self, flags, pipeline},
	math::Vector,
};

#[derive(Debug, Clone, Copy)]
pub struct Vertex {
	pub pos_and_width_edge: Vector<f32, 4>,
	pub tex_coord: Vector<f32, 4>,
	pub color: Vector<f32, 4>,
}

impl pipeline::vertex::Object for Vertex {
	fn attributes() -> Vec<pipeline::vertex::Attribute> {
		vec![
			pipeline::vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, pos_and_width_edge),
				format: flags::format::VEC4,
			},
			pipeline::vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, tex_coord),
				format: flags::format::VEC2,
			},
			pipeline::vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, color),
				format: flags::format::VEC4,
			},
		]
	}
}

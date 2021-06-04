use crate::{
	graphics::{self, flags, pipeline::state::vertex},
	math::nalgebra::Vector4,
};

#[derive(Debug, Clone, Copy)]
pub struct Vertex {
	pub pos_and_width_edge: Vector4<f32>,
	pub tex_coord: Vector4<f32>,
	pub color: Vector4<f32>,
}

impl vertex::Object for Vertex {
	fn attributes() -> Vec<vertex::Attribute> {
		vec![
			vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, pos_and_width_edge),
				format: flags::format::VEC4,
			},
			vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, tex_coord),
				format: flags::format::VEC2,
			},
			vertex::Attribute {
				offset: graphics::utility::offset_of!(Vertex, color),
				format: flags::format::VEC4,
			},
		]
	}
}

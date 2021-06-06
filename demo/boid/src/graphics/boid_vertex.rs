use crate::engine::{
	graphics::{flags, pipeline, types::Vec2, vertex_object},
	math::nalgebra::Vector2,
};

#[vertex_object]
#[derive(Debug, Default)]
pub struct Vertex {
	#[vertex_attribute([R, G], Bit32, SFloat)]
	pos: Vec2,

	#[vertex_attribute([R, G], Bit32, SFloat)]
	tex_coord: Vec2,
}

impl Vertex {
	pub fn with_pos(mut self, pos: Vector2<f32>) -> Self {
		self.pos = pos.into();
		self
	}
	pub fn with_tex_coord(mut self, texture_coordinate: Vector2<f32>) -> Self {
		self.tex_coord = texture_coordinate.into();
		self
	}
}

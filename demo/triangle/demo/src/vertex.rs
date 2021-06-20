use crate::engine::{
	graphics::{
		flags, pipeline,
		types::{Vec2, Vec4},
		vertex_object,
	},
	math::nalgebra::{Vector2, Vector4},
};

#[vertex_object]
#[derive(Debug, Default)]
pub struct Vertex {
	#[vertex_attribute([R, G], Bit32, SFloat)]
	pos: Vec2,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	color: Vec4,
}

#[cfg(test)]
mod vertex_data {
	use super::*;

	#[test]
	fn alignment() {
		assert_eq!(crate::engine::graphics::utility::offset_of!(Vertex, pos), 0);
		assert_eq!(
			crate::engine::graphics::utility::offset_of!(Vertex, color),
			16
		);
	}
}

impl Vertex {
	pub fn with_pos(mut self, pos: Vector2<f32>) -> Self {
		self.pos = pos.into();
		self
	}
	pub fn with_color(mut self, color: Vector4<f32>) -> Self {
		self.color = color.into();
		self
	}
}

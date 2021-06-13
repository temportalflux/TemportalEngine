use crate::engine::{
	graphics::{
		flags, pipeline,
		types::{Mat4, Vec4},
		vertex_object,
	},
	math::nalgebra::{Matrix4, UnitQuaternion, Vector3, Vector4},
	world,
};
use std::ops::DerefMut;

#[vertex_object]
#[derive(Debug, Default)]
pub struct Instance {
	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	#[vertex_span(4)]
	pub model: Mat4,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	pub color: Vec4,
}

impl Instance {
	pub fn with_pos(mut self, pos: Vector3<f32>) -> Self {
		*self.model.deref_mut() *= Matrix4::new_translation(&pos);
		self
	}

	pub fn with_orientation(mut self, orientation: UnitQuaternion<f32>) -> Self {
		let rot_matrix: Matrix4<f32> =
			UnitQuaternion::from_axis_angle(&world::global_forward(), 180_f32.to_radians()).into();
		*self.model.deref_mut() *= rot_matrix;
		let rot_matrix: Matrix4<f32> = orientation.into();
		*self.model.deref_mut() *= rot_matrix;
		self
	}

	pub fn with_color(mut self, color: Vector4<f32>) -> Self {
		self.color = color.into();
		self
	}
}

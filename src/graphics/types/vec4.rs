use crate::math::nalgebra::Vector4;

#[derive(Debug, Default, Clone)]
// technically redundant since `[f32; 4]` is already aligned to 16 bytes, but kept for the sake of consistency.
#[repr(C, align(16))]
pub struct Vec4(Vector4<f32>);

impl std::ops::Deref for Vec4 {
	type Target = Vector4<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl From<Vector4<f32>> for Vec4 {
	fn from(vec: Vector4<f32>) -> Self {
		Self(vec)
	}
}

impl From<[f32; 4]> for Vec4 {
	fn from(vec: [f32; 4]) -> Self {
		Self(vec.into())
	}
}

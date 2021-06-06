use crate::math::nalgebra::Vector3;

#[derive(Debug, Default, Clone)]
#[repr(C, align(16))]
pub struct Vec3(Vector3<f32>);

impl std::ops::Deref for Vec3 {
	type Target = Vector3<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl From<Vector3<f32>> for Vec3 {
	fn from(vec: Vector3<f32>) -> Self {
		Self(vec)
	}
}

impl From<[f32; 3]> for Vec3 {
	fn from(vec: [f32; 3]) -> Self {
		Self(vec.into())
	}
}

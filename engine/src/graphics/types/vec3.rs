use crate::math::nalgebra::{Point3, Vector3};
use serde::{Deserialize, Serialize};

/// An array of 3 f32 values, aligned to a 16-byte boundary,
/// (so it takes up 16 bytes instead of 12 bytes).
///
/// Often used to represent 3-dimensional positions or RGB color.
#[derive(Debug, Default, Clone, PartialEq, Deserialize, Serialize)]
#[repr(C, align(16))]
pub struct Vec3(Vector3<f32>);

/// Returns the underlying [`Vector3`] nalgebra vector.
impl std::ops::Deref for Vec3 {
	type Target = Vector3<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

/// Wraps the provided [`Vector3`] nalgebra vector.
impl From<Vector3<f32>> for Vec3 {
	fn from(vec: Vector3<f32>) -> Self {
		Self(vec)
	}
}

/// Wraps the provided [`Point3`] nalgebra vector.
impl From<Point3<f32>> for Vec3 {
	fn from(vec: Point3<f32>) -> Self {
		Self(Vector3::new(vec.x, vec.y, vec.z))
	}
}

impl From<[f32; 3]> for Vec3 {
	fn from(vec: [f32; 3]) -> Self {
		Self(vec.into())
	}
}

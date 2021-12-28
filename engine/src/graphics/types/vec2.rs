use crate::math::nalgebra::{Point2, Vector2};

/// An array of 2 f32 values, aligned to a 16-byte boundary,
/// (so it takes up 16 bytes instead of 8 bytes).
///
/// Often used to represent 2-dimensional positions or texture coordinates.
#[derive(Debug, Default, Clone)]
#[repr(C, align(16))]
pub struct Vec2(Vector2<f32>);

/// Returns the underlying [`Vector2`] nalgebra vector.
impl std::ops::Deref for Vec2 {
	type Target = Vector2<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

/// Wraps the provided [`Vector2`] nalgebra vector.
impl From<Vector2<f32>> for Vec2 {
	fn from(vec: Vector2<f32>) -> Self {
		Self(vec)
	}
}

/// Wraps the provided [`Point2`] nalgebra vector.
impl From<Point2<f32>> for Vec2 {
	fn from(vec: Point2<f32>) -> Self {
		Self(Vector2::new(vec.x, vec.y))
	}
}

impl From<[f32; 2]> for Vec2 {
	fn from(vec: [f32; 2]) -> Self {
		Self(vec.into())
	}
}

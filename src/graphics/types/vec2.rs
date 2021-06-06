use crate::math::nalgebra::Vector2;

#[derive(Debug, Default, Clone)]
#[repr(C, align(16))]
pub struct Vec2(Vector2<f32>);

impl std::ops::Deref for Vec2 {
	type Target = Vector2<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl From<Vector2<f32>> for Vec2 {
	fn from(vec: Vector2<f32>) -> Self {
		Self(vec)
	}
}

impl From<[f32; 2]> for Vec2 {
	fn from(vec: [f32; 2]) -> Self {
		Self(vec.into())
	}
}

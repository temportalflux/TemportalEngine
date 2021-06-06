use crate::math::nalgebra::Matrix4;

/// An array of 16 f32 values, aligned to a 64-byte boundary.
///
/// Often used to represent a transformational matrix.
#[derive(Debug, Clone)]
// technically redundant since `[f32; 16]` is already aligned to 64 bytes, but kept for the sake of consistency.
#[repr(C, align(16))]
pub struct Mat4(Matrix4<f32>);

impl Default for Mat4 {
	fn default() -> Self {
		Self(Matrix4::identity())
	}
}

/// Returns the underlying [`Matrix4`] nalgebra vector.
impl std::ops::Deref for Mat4 {
	type Target = Matrix4<f32>;
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

/// Returns the underlying [`Matrix4`] nalgebra vector.
impl std::ops::DerefMut for Mat4 {
	fn deref_mut(&mut self) -> &mut Self::Target {
		&mut self.0
	}
}

/// Wraps the provided [`Matrix4`] nalgebra vector.
impl From<Matrix4<f32>> for Mat4 {
	fn from(vec: Matrix4<f32>) -> Self {
		Self(vec)
	}
}

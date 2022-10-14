use crate::math::nalgebra::{Unit, Vector3};

// TODO: I think the X axis in CS is flipped.
// https://www.evl.uic.edu/ralph/508S98/coordinates.html
// When navigating the world, the positive X is accidentally left somehow.

/// The unit vector for the world forward direction.
/// Y-Up Right-Handed is +X, +Y, -Z
/// In 2D space, forward points out of the screen at the user.
pub fn global_forward() -> Unit<Vector3<f32>> {
	-Vector3::<f32>::z_axis()
}

/// The unit vector for the world right direction.
/// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_right() -> Unit<Vector3<f32>> {
	Vector3::<f32>::x_axis()
}

/// The unit vector for the world up direction.
/// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_up() -> Unit<Vector3<f32>> {
	Vector3::<f32>::y_axis()
}

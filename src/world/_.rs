use temportal_math::Vector;

/// The unit vector for the world forward direction.
/// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_forward() -> Vector<f32, 3> {
	Vector::new([0.0, 0.0, -1.0])
}

/// The unit vector for the world right direction.
/// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_right() -> Vector<f32, 3> {
	Vector::new([1.0, 0.0, 0.0])
}

/// The unit vector for the world up direction.
/// Y-Up Right-Handed is +X, +Y, -Z
pub fn global_up() -> Vector<f32, 3> {
	Vector::new([0.0, 1.0, 0.0])
}

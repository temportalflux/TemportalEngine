use crate::{
	math::{Matrix, Quaternion, Vector},
	world,
};

#[derive(Default, Debug, Clone, Copy)]
pub struct Camera {
	position: Vector<f32, 3>,
	orientation: Quaternion,
	projection: Projection,
}

#[derive(Debug, Clone, Copy)]
pub enum Projection {
	Orthographic(OrthographicBounds),
	Perspective(PerspectiveProjection),
}

impl Default for Projection {
	fn default() -> Self {
		Self::Orthographic(Default::default())
	}
}

#[derive(Debug, Default, Clone, Copy)]
pub struct OrthographicBounds {
	pub x: Vector<f32, 2>,
	pub y: Vector<f32, 2>,
	pub z: Vector<f32, 2>,
}

#[derive(Debug, Default, Clone, Copy)]
pub struct PerspectiveProjection {
	vertical_fov: f32,
	near_plane: f32,
	far_plane: f32,
}

#[derive(Debug, Clone, Copy)]
pub struct ViewProjection {
	view: Matrix<f32, 4, 4>,
	projection: Matrix<f32, 4, 4>,
}

impl Default for ViewProjection {
	fn default() -> Self {
		use crate::math::Identity;
		Self {
			view: Matrix::identity(),
			projection: Matrix::identity(),
		}
	}
}

impl Camera {
	pub fn with_position(mut self, pos: Vector<f32, 3>) -> Self {
		self.set_position(pos);
		self
	}

	pub fn set_position(&mut self, pos: Vector<f32, 3>) {
		self.position = pos;
	}

	pub fn with_orientation(mut self, orientation: Quaternion) -> Self {
		self.set_orientation(orientation);
		self
	}

	pub fn set_orientation(&mut self, orientation: Quaternion) {
		self.orientation = orientation;
	}

	pub fn with_projection(mut self, projection: Projection) -> Self {
		self.set_projection(projection);
		self
	}

	pub fn set_projection(&mut self, projection: Projection) {
		self.projection = projection;
	}

	pub fn as_uniform_matrix(&self, resolution: Vector<f32, 2>) -> ViewProjection {
		let forward = self.orientation.rotate(&world::global_forward());
		let up = self.orientation.rotate(&world::global_up());
		let aspect_ratio = (resolution.x() as f32) / (resolution.y() as f32);
		ViewProjection {
			view: Matrix::look_at(self.position, self.position + forward, up),
			projection: self.projection_matrix(aspect_ratio),
		}
	}

	fn projection_matrix(&self, aspect_ratio: f32) -> Matrix<f32, 4, 4> {
		match &self.projection {
			Projection::Orthographic(bounds) => Matrix::orthographic(
				bounds.x.x(),
				bounds.x.y(),
				bounds.y.x(),
				bounds.y.y(),
				bounds.z.x(),
				bounds.z.y(),
			),
			Projection::Perspective(proj) => Matrix::perspective_right_hand_depth_zero_to_one(
				Self::vertical_to_horizontal_fov(proj.vertical_fov, aspect_ratio),
				aspect_ratio,
				proj.near_plane,
				proj.far_plane,
			),
		}
	}

	fn vertical_to_horizontal_fov(vertical: f32, xy_aspect_ratio: f32) -> f32 {
		// According to this calculator http://themetalmuncher.github.io/fov-calc/
		// whose source code is https://github.com/themetalmuncher/fov-calc/blob/gh-pages/index.html#L24
		// the equation to get verticalFOV from horizontalFOV is: verticalFOV = 2 * atan(tan(horizontalFOV / 2) * height / width)
		// And by shifting the math to get horizontal from vertical, the equation is actually the same except the aspectRatio is flipped.
		2.0 * f32::atan(f32::tan(vertical / 2.0) * xy_aspect_ratio)
	}
}

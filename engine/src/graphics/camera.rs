use crate::{
	math::nalgebra::{Isometry3, Translation3, Matrix4, Point3, UnitQuaternion, Vector2},
	world,
};

pub trait Camera {
	fn position(&self) -> &Point3<f32>;
	fn orientation(&self) -> &UnitQuaternion<f32>;
	fn projection(&self) -> &Projection;

	fn view_matrix(&self) -> Matrix4<f32> {
		let position = self.position();
		let orientation = self.orientation();
		let forward = orientation * world::global_forward();
		let up = orientation * world::global_up();
		let target = position + forward.into_inner();
		Isometry3::look_at_rh(&position, &target, &up).to_homogeneous()
	}

	fn projection_matrix(&self, resolution: &Vector2<f32>) -> Matrix4<f32> {
		let aspect_ratio = resolution.x / resolution.y;
		self.projection().as_matrix(aspect_ratio)
	}
}

#[derive(Debug, Clone, Copy)]
pub struct DefaultCamera {
	position: Point3<f32>,
	orientation: UnitQuaternion<f32>,
	projection: Projection,
}

impl Default for DefaultCamera {
	fn default() -> Self {
		Self {
			position: [0.0, 0.0, 0.0].into(),
			orientation: UnitQuaternion::<f32>::identity(),
			projection: Projection::default(),
		}
	}
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

impl std::fmt::Display for Projection {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match self {
			Self::Orthographic(ortho) => write!(
				f,
				"Orthographic(x=<{}, {}> y=<{}, {}> z=<{}, {}>)",
				ortho.x[0], ortho.x[1], ortho.y[0], ortho.y[1], ortho.z[0], ortho.z[1],
			),
			Self::Perspective(persp) => write!(
				f,
				"Perspective(fov={} near={} far={})",
				persp.vertical_fov, persp.near_plane, persp.far_plane
			),
		}
	}
}

#[derive(Debug, Default, Clone, Copy)]
pub struct OrthographicBounds {
	pub x: Vector2<f32>,
	pub y: Vector2<f32>,
	pub z: Vector2<f32>,
}

impl OrthographicBounds {
	pub fn left(&self) -> f32 {
		self.x.x
	}
	pub fn right(&self) -> f32 {
		self.x.y
	}
	pub fn bottom(&self) -> f32 {
		self.y.x
	}
	pub fn top(&self) -> f32 {
		self.y.y
	}
	pub fn z_near(&self) -> f32 {
		self.z.x
	}
	pub fn z_far(&self) -> f32 {
		self.z.y
	}
	pub fn as_matrix(&self) -> Matrix4<f32> {
		Projection::orthographic(
			self.left(),
			self.right(),
			self.bottom(),
			self.top(),
			self.z_near(),
			self.z_far(),
		)
	}
}

#[derive(Debug, Default, Clone, Copy)]
pub struct PerspectiveProjection {
	pub vertical_fov: f32,
	pub near_plane: f32,
	pub far_plane: f32,
}

impl PerspectiveProjection {
	pub fn as_matrix(&self, aspect_ratio: f32) -> Matrix4<f32> {
		/*
		Projection::perspective_right_hand_depth_zero_to_one(
			Projection::vertical_to_horizontal_fov(self.vertical_fov, aspect_ratio),
			aspect_ratio,
			self.near_plane,
			self.far_plane,
		)
		*/
		nalgebra::geometry::Perspective3::new(
			aspect_ratio,
			self.vertical_fov,
			self.near_plane,
			self.far_plane,
		)
		.to_homogeneous()
	}
}

impl Projection {
	pub fn as_matrix(&self, aspect_ratio: f32) -> Matrix4<f32> {
		match self {
			Self::Orthographic(bounds) => bounds.as_matrix(),
			Self::Perspective(proj) => proj.as_matrix(aspect_ratio),
		}
	}

	pub fn orthographic(
		left: f32,
		right: f32,
		bottom: f32,
		top: f32,
		z_near: f32,
		z_far: f32,
	) -> Matrix4<f32> {
		let mut matrix = Matrix4::<f32>::identity();
		matrix[(0, 0)] = 2.0 / (right - left);
		matrix[(1, 1)] = 2.0 / (top - bottom);
		matrix[(2, 2)] = 1.0 / (z_far - z_near);
		matrix[(0, 3)] = -(right + left) / (right - left);
		matrix[(1, 3)] = -(top + bottom) / (top - bottom);
		matrix[(2, 3)] = -z_near / (z_far - z_near);
		matrix
	}

	pub fn vertical_to_horizontal_fov(vertical: f32, xy_aspect_ratio: f32) -> f32 {
		// According to this calculator http://themetalmuncher.github.io/fov-calc/
		// whose source code is https://github.com/themetalmuncher/fov-calc/blob/gh-pages/index.html#L24
		// the equation to get verticalFOV from horizontalFOV is: verticalFOV = 2 * atan(tan(horizontalFOV / 2) * height / width)
		// And by shifting the math to get horizontal from vertical, the equation is actually the same except the aspectRatio is flipped.
		2.0 * f32::atan(f32::tan(vertical / 2.0) * xy_aspect_ratio)
	}

	pub fn perspective_right_hand_depth_zero_to_one(
		y_fov: f32,
		aspect_ratio: f32,
		near_plane: f32,
		far_plane: f32,
	) -> Matrix4<f32> {
		// Based on GLM https://docs.rs/nalgebra-glm/0.13.0/src/nalgebra_glm/ext/matrix_clip_space.rs.html#665-689
		// A tweet about handedness in different engines: https://twitter.com/FreyaHolmer/status/644881436982575104
		assert!(f32::abs(aspect_ratio - f32::EPSILON) > 0.0);
		let tan_half_fov_y = f32::tan(y_fov / 2.0);
		let mut perspective = Matrix4::<f32>::default();
		perspective[(0, 0)] = 1.0 / (aspect_ratio * tan_half_fov_y);
		perspective[(1, 1)] = 1.0 / (tan_half_fov_y);
		perspective[(2, 2)] = far_plane / (near_plane - far_plane);
		perspective[(3, 2)] = -1.0;
		perspective[(2, 3)] = -(far_plane * near_plane) / (far_plane - near_plane);
		return perspective;
	}
}

#[allow(dead_code)]
#[derive(Debug, Clone, Copy)]
pub struct ViewProjection {
	view: Matrix4<f32>,
	projection: Matrix4<f32>,
}

impl Default for ViewProjection {
	fn default() -> Self {
		Self {
			view: Matrix4::identity(),
			projection: Matrix4::identity(),
		}
	}
}

impl DefaultCamera {
	pub fn with_position(mut self, pos: Point3<f32>) -> Self {
		self.set_position(pos);
		self
	}

	pub fn set_position(&mut self, pos: Point3<f32>) {
		self.position = pos;
	}

	pub fn with_orientation(mut self, orientation: UnitQuaternion<f32>) -> Self {
		self.set_orientation(orientation);
		self
	}

	pub fn set_orientation(&mut self, orientation: UnitQuaternion<f32>) {
		self.orientation = orientation;
	}

	pub fn with_projection(mut self, projection: Projection) -> Self {
		self.set_projection(projection);
		self
	}

	pub fn set_projection(&mut self, projection: Projection) {
		self.projection = projection;
	}

	pub fn as_uniform_matrix(&self, resolution: &Vector2<f32>) -> ViewProjection {
		ViewProjection {
			view: self.view_matrix(),
			projection: self.projection_matrix(resolution),
		}
	}
}

impl Camera for DefaultCamera {
	fn position(&self) -> &Point3<f32> {
		&self.position
	}
	fn orientation(&self) -> &UnitQuaternion<f32> {
		&self.orientation
	}
	fn projection(&self) -> &Projection {
		&self.projection
	}
}

use crate::{
	math::nalgebra::{Isometry3, Matrix4, Point3, UnitQuaternion, Vector2},
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
		let perspective = nalgebra::geometry::Perspective3::new(
			aspect_ratio,
			self.vertical_fov,
			self.near_plane,
			self.far_plane,
		);
		let mut projection = perspective.to_homogeneous();
		/*
		According to https://github.com/dimforge/nalgebra/issues/204:
			"The choice of nalgebra is to flip the z-axis in order to switch to
			a left-handed coordinate system. This is motivated by OpenGL-based applications."
		and https://www.nalgebra.org/docs/user_guide/projections:
			"The actual shape to be transformed depends on the projection itself.
			Note that projections implemented on nalgebra also flip the z axis. This is a common convention
			in computer graphics applications for rendering with, e.g., OpenGL,
			because the coordinate system of the screen is left-handed.
		In order to correct for this, we need to flip the x-modifier to transform to our
		right-handed -Z forward +X right coordinate space.
		https://www.evl.uic.edu/ralph/508S98/coordinates.html
		*/
		projection[(0, 0)] *= -1.0;
		projection
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

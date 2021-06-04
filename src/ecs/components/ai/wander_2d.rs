use crate::{
	ecs::{components::ai::steering, Component, VecStorage},
	math::nalgebra::{Point3, UnitQuaternion},
	rand::{self, Rng},
	world,
};
use serde::{Deserialize, Serialize};

/// Settings for the wander behavior explained in
/// "Artificial Intelligence for Games: 2nd Edition" by Ian Millington.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Wander2D {
	/// The radius of the circle projected in front of the entity's position,
	/// on the circumfrence of which the wander target exists.
	radius: f32,
	/// The distance the wander-circle is projected in front of the entity's position,
	/// on the circumfrence of which the wander target exists.
	projection_distance: f32,
	/// How fast, in radians, the wander's target moves around the
	/// circumfrence of the wander circle.
	rate_of_change: f32,
	// How fast the entity should move towards its wander target.
	linear_speed: f32,

	face: steering::Face,

	// DEBUG
	pub(crate) target_position: Point3<f32>,
}

impl Component for Wander2D {
	type Storage = VecStorage<Self>;
}

impl Default for Wander2D {
	fn default() -> Self {
		Self {
			radius: 1.0,
			projection_distance: 2.0,
			rate_of_change: 180.0_f32.to_radians(),
			linear_speed: 1.0,
			face: steering::Face::default(),
			target_position: [0.0, 0.0, 0.0].into(),
		}
	}
}

impl Wander2D {
	/// Sets the radius of the wander circle
	/// (a circle projected in front of the entity's position,
	/// on the circumfrence of which the wander target is placed).
	pub fn with_circle_radius(mut self, radius: f32) -> Self {
		self.radius = radius;
		self
	}

	pub fn circle_radius(&self) -> f32 {
		self.radius
	}

	/// Sets the distance the wander circle is projected in front of the entity.
	/// (a circle projected in front of the entity's position,
	/// on the circumfrence of which the wander target is placed).
	pub fn with_projection_distance(mut self, distance: f32) -> Self {
		self.projection_distance = distance;
		self
	}

	pub fn projection_distance(&self) -> f32 {
		self.projection_distance
	}

	pub fn with_rate_of_change(mut self, radians: f32) -> Self {
		self.rate_of_change = radians;
		self
	}

	pub fn target_rate_of_change(&self) -> f32 {
		self.rate_of_change
	}

	pub fn with_linear_speed(mut self, speed: f32) -> Self {
		self.linear_speed = speed;
		self
	}

	pub fn with_face(mut self, face: steering::Face) -> Self {
		self.face = face;
		self
	}
}

impl steering::Behavior for Wander2D {
	fn name() -> &'static str {
		"wander_2d"
	}

	fn get_steering(&mut self, state: &steering::State) -> steering::Output {
		let mut rng = rand::thread_rng();
		// To determine the "target" location, we need a point on the circumfrence of the circle.
		// Such a point would be determined by a rotation around the circle at a given radius.
		// We don't want the target to move too drastically, so we will use the entity's current
		// orientation + some rotational rate around the circle to determine the rotation around the cirlce.
		let random_binomial = rng.gen::<f32>() - rng.gen::<f32>();
		let target_orientation = state.orientation
			* UnitQuaternion::from_axis_angle(
				&world::global_forward(),
				self.target_rate_of_change() * random_binomial,
			);

		let forward = state.orientation * world::global_up().into_inner();

		// normalized vector from circle center in the direction of the target on the circle edge
		let target_forward = target_orientation * world::global_up().into_inner();
		// the center of the wander circle - which is a projection in front of the position of the entity
		let circle_center = state.position + forward * self.projection_distance();
		// world position of the target on the edge of the circle
		let target_position = circle_center + target_forward * self.circle_radius();

		//log::debug!("{}", target_forward - forward);
		self.target_position = target_position;

		let mut steering = self.face.get_steering(state, target_position);
		steering.linear_acceleration = forward * self.linear_speed;
		steering
	}
}

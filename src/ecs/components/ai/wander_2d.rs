use crate::{
	ecs::{Component, VecStorage},
	math::Quaternion,
};
use serde::{Deserialize, Serialize};

/// Settings for the wander behavior explained in
/// "Artificial Intelligence for Games: 2nd Edition" by Ian Millington.
/// Functionality handled by [`WanderIn2D`](crate::ecs::systems::ai::WanderIn2D).
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
	// How fast the entity should adjust its orientation
	angular_speed: f32,

	pub(crate) target_orientation: Quaternion,
	pub(crate) entity_desired_orientation: Quaternion,
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
			angular_speed: 1.0,
			target_orientation: Quaternion::identity(),
			entity_desired_orientation: Quaternion::identity(),
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

	pub fn linear_speed(&self) -> f32 {
		self.linear_speed
	}

	pub fn with_angular_speed(mut self, speed: f32) -> Self {
		self.angular_speed = speed;
		self
	}

	pub fn angular_speed(&self) -> f32 {
		self.angular_speed
	}
}
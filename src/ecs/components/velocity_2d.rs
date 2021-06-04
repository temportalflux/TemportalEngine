use crate::{
	ecs::{Component, VecStorage},
	math::nalgebra::{Vector2, Vector3},
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Velocity2D(Vector2<f32>);

impl Component for Velocity2D {
	type Storage = VecStorage<Self>;
}

impl Velocity2D {
	pub fn new(velocity: Vector2<f32>) -> Self {
		Self(velocity)
	}

	pub fn get(&self) -> Vector2<f32> {
		self.0
	}

	pub fn get3(&self) -> Vector3<f32> {
		Vector3::new(self.0.x, self.0.y, 0.0)
	}

	pub fn apply(&mut self, vec: Vector2<f32>) {
		self.0 += vec;
	}

	pub fn apply_max_speed(&mut self, speed: f32) {
		if self.0.magnitude_squared() > speed.powi(2) {
			self.0 = self.0.normalize() * speed;
		}
	}
}

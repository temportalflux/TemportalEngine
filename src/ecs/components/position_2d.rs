use crate::{
	ecs::{Component, VecStorage},
	math::nalgebra::{Point2, Point3, Vector2},
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Position2D(Point2<f32>);

impl Component for Position2D {
	type Storage = VecStorage<Self>;
}

impl Position2D {
	pub fn new(position: Point2<f32>) -> Self {
		Self(position)
	}

	pub fn get(&self) -> Point2<f32> {
		self.0
	}

	pub fn get3(&self) -> Point3<f32> {
		Point3::new(self.0.x, self.0.y, 0.0)
	}

	pub fn apply(&mut self, vec: Vector2<f32>) {
		self.0 += vec;
	}

	pub fn apply_bounds(&mut self, min: &Vector2<f32>, max: &Vector2<f32>) {
		for i in 0..self.0.len() {
			let range = max[i] - min[i];
			while self.0[i] >= max[i] {
				self.0[i] -= range;
			}
			while self.0[i] < min[i] {
				self.0[i] += range;
			}
		}
	}
}

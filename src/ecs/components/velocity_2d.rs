use crate::{
	ecs::{Component, VecStorage},
	math::Vector,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Velocity2D(pub Vector<f32, 2>);

impl Component for Velocity2D {
	type Storage = VecStorage<Self>;
}

impl Velocity2D {
	pub fn get(&self) -> Vector<f32, 2> {
		self.0
	}

	pub fn set(&mut self, velocity: Vector<f32, 2>) {
		self.0 = velocity;
	}
}

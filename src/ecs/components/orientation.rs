use crate::{
	ecs::{Component, VecStorage},
	math::nalgebra::{UnitQuaternion, Vector3},
	world,
};
use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Serialize, Deserialize)]
pub struct Orientation(UnitQuaternion<f32>);

impl Component for Orientation {
	type Storage = VecStorage<Self>;
}

impl Orientation {
	pub fn new(orientation: UnitQuaternion<f32>) -> Self {
		Self(orientation)
	}

	pub fn get(&self) -> UnitQuaternion<f32> {
		self.0
	}

	pub fn apply(&mut self, rotation: UnitQuaternion<f32>) {
		self.0 *= rotation;
	}

	pub fn forward(&self) -> Vector3<f32> {
		self.get() * world::global_forward().into_inner()
	}

	pub fn right(&self) -> Vector3<f32> {
		self.get() * world::global_right().into_inner()
	}

	pub fn up(&self) -> Vector3<f32> {
		self.get() * world::global_up().into_inner()
	}
}

use crate::{
	ecs::{Component, VecStorage},
	math::{Quaternion, Vector},
	world,
};
use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Serialize, Deserialize)]
pub struct Orientation(pub Quaternion);

impl Component for Orientation {
	type Storage = VecStorage<Self>;
}

impl Orientation {
	pub fn rotate_by(&mut self, rotation: Quaternion) {
		self.0 = Quaternion::concat(&self.0, &rotation);
	}

	pub fn get(&self) -> &Quaternion {
		&self.0
	}

	pub fn set(&mut self, orientation: Quaternion) {
		self.0 = orientation;
	}

	pub fn up(&self) -> Vector<f32, 3> {
		self.get().rotate(&world::global_up())
	}
}

use crate::{
	ecs::{Component, VecStorage},
	math::Quaternion,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Orientation(pub Quaternion);

impl Component for Orientation {
	type Storage = VecStorage<Self>;
}

impl Default for Orientation {
	fn default() -> Orientation {
		Orientation(Quaternion::identity())
	}
}

impl Orientation {
	pub fn rotate(&mut self, rotation: Quaternion) {
		self.0 = Quaternion::concat(&self.0, &rotation);
	}
	pub fn get(&self) -> &Quaternion {
		&self.0
	}
}

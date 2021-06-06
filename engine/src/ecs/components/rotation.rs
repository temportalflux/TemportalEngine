use crate::ecs::{Component, VecStorage};
use serde::{Deserialize, Serialize};

// Angular Velocity
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Rotation(pub f32);

impl Component for Rotation {
	type Storage = VecStorage<Self>;
}

impl Default for Rotation {
	fn default() -> Self {
		Self(0.0)
	}
}

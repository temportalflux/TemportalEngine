use crate::{
	ecs::{Component, VecStorage},
	math::Quaternion,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Orientation(Quaternion);

impl Component for Orientation {
	type Storage = VecStorage<Self>;
}

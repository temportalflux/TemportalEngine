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

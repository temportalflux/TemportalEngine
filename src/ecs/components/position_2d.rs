use crate::{
	ecs::{Component, VecStorage},
	math::Vector,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Position2D(pub Vector<f32, 2>);

impl Component for Position2D {
	type Storage = VecStorage<Self>;
}

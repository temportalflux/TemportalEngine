use crate::{
	ecs::{Component, VecStorage},
	engine::math::nalgebra::Vector4,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BoidRender {
	pub color: Vector4<f32>,
}

impl Component for BoidRender {
	type Storage = VecStorage<Self>;
}

impl BoidRender {
	pub fn new(color: Vector4<f32>) -> Self {
		Self { color }
	}
}

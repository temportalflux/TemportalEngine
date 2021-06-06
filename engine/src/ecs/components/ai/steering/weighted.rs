use crate::ecs::{components::ai::steering, Component, VecStorage};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Weighted {
	weights: HashMap<steering::IdOwned, f32>,
	max_linear_speed: f32,
}

impl Component for Weighted {
	type Storage = VecStorage<Self>;
}

impl Default for Weighted {
	fn default() -> Self {
		Self {
			weights: HashMap::new(),
			max_linear_speed: 0.0,
		}
	}
}

impl Weighted {
	pub fn with<T: steering::Behavior>(mut self, weight: f32) -> Self {
		self.weights.insert(T::name_owned(), weight);
		self
	}

	pub fn get_weight<T: steering::Behavior>(&self) -> Option<&f32> {
		self.weights.get(&T::name_owned())
	}

	pub fn with_max_linear(mut self, speed: f32) -> Self {
		self.max_linear_speed = speed;
		self
	}

	pub fn max_linear_speed(&self) -> f32 {
		self.max_linear_speed
	}
}

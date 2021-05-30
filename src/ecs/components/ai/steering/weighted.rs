use crate::ecs::{components::ai::steering, Component, VecStorage};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Weighted {
	weights: HashMap<steering::IdOwned, f32>,
	max_linear_acceleration: f32,
}

impl Component for Weighted {
	type Storage = VecStorage<Self>;
}

impl Weighted {
	pub fn new(max_linear_acceleration: f32) -> Self {
		Self {
			weights: HashMap::new(),
			max_linear_acceleration,
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

	pub fn max_linear_acceleration(&self) -> f32 {
		self.max_linear_acceleration
	}
}

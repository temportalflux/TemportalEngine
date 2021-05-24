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

impl std::ops::Deref for Position2D {
	type Target = Vector<f32, 2>;

	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl std::ops::DerefMut for Position2D {
	fn deref_mut(&mut self) -> &mut Self::Target {
		&mut self.0
	}
}

impl Position2D {
	pub fn get(&self) -> &Vector<f32, 2> {
		&self.0
	}
}

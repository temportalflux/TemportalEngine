use crate::{
	ecs::{self, components::Position2D, NamedSystem},
	engine::math::nalgebra::Vector2,
};

#[derive(Default)]
pub struct WorldBounds {
	min: Vector2<f32>,
	max: Vector2<f32>,
}

impl NamedSystem for WorldBounds {
	fn name() -> &'static str {
		"world_bounds"
	}
	fn dependencies(&self) -> Vec<&'static str> {
		vec![ecs::systems::MoveEntities::name()]
	}
}

impl WorldBounds {
	pub fn with_bounds(mut self, min: Vector2<f32>, max: Vector2<f32>) -> Self {
		self.min = min;
		self.max = max;
		self
	}
}

impl<'a> ecs::System<'a> for WorldBounds {
	type SystemData = ecs::WriteStorage<'a, Position2D>;

	fn run(&mut self, mut position_store: Self::SystemData) {
		use ecs::Join;
		for position in (&mut position_store).join() {
			position.apply_bounds(&self.min, &self.max);
		}
	}
}

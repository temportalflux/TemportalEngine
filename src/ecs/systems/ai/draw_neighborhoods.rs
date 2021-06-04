use crate::{
	ecs::{
		self,
		components::{ai::steering::Neighborhood, Position2D},
		Join, NamedSystem,
	},
	input,
	math::nalgebra::vector,
	rand, render, world,
};
use std::sync::{Arc, RwLock};

pub struct DrawNeighborhoods {
	debug_render: Arc<RwLock<render::DebugRender>>,
	action_select_prev: Option<input::action::Id>,
	action_select_next: Option<input::action::Id>,
	action_select_none: Option<input::action::Id>,
	selected_entity_id: Option<ecs::world::Index>,
	max_encountered_id: ecs::world::Index,
}

impl DrawNeighborhoods {
	pub fn new(debug_render: Arc<RwLock<render::DebugRender>>) -> Self {
		Self {
			debug_render,
			action_select_prev: None,
			action_select_next: None,
			action_select_none: None,
			selected_entity_id: None,
			max_encountered_id: 0,
		}
	}

	pub fn with_select_actions(
		mut self,
		prev: input::action::Id,
		next: input::action::Id,
		none: input::action::Id,
	) -> Self {
		self.action_select_prev = Some(prev);
		self.action_select_next = Some(next);
		self.action_select_none = Some(none);
		self
	}

	pub fn on_pressed(&self, action: Option<input::action::Id>) -> bool {
		if let Some(action_id) = action {
			if let Some(action) = input::read().get_user_action(0, action_id) {
				if action.on_button_pressed() {
					return true;
				}
			}
		}
		false
	}

	fn random_entity_id(&self) -> ecs::world::Index {
		use rand::Rng;
		rand::thread_rng().gen_range(0..self.max_encountered_id)
	}
}

impl NamedSystem for DrawNeighborhoods {
	fn name() -> &'static str {
		"draw_neighborhoods"
	}
}

impl<'a> ecs::System<'a> for DrawNeighborhoods {
	type SystemData = (
		ecs::Entities<'a>,
		ecs::ReadStorage<'a, Neighborhood>,
		ecs::ReadStorage<'a, Position2D>,
	);

	fn run(&mut self, (store_entities, store_neighborhood, store_position): Self::SystemData) {
		if self.on_pressed(self.action_select_prev) {
			self.selected_entity_id = self
				.selected_entity_id
				.map(|id| id - 1)
				.or(Some(self.random_entity_id()));
		}
		if self.on_pressed(self.action_select_next) {
			self.selected_entity_id = self
				.selected_entity_id
				.map(|id| id + 1)
				.or(Some(self.random_entity_id()));
		}
		if self.on_pressed(self.action_select_none) {
			self.selected_entity_id = None;
		}

		let mut drawer = self.debug_render.write().unwrap();
		for (entity, neighborhood, position) in
			(&store_entities, &store_neighborhood, &store_position).join()
		{
			if entity.id() > self.max_encountered_id {
				self.max_encountered_id = entity.id();
			}

			if let Some(selected_id) = &self.selected_entity_id {
				if entity.id() != *selected_id {
					continue;
				}

				drawer.draw_segment(
					render::Point {
						position: position.get3(),
						color: vector![0.0, 1.0, 0.0, 1.0],
					},
					render::Point {
						position: position.get3() - world::global_up().into_inner(),
						color: vector![0.0, 1.0, 0.0, 1.0],
					},
				);
			}

			for neighbor_position in neighborhood.neighbors.iter().filter_map(|neighbor_ent_id| {
				let neighbor = store_entities.entity(*neighbor_ent_id);
				if store_entities.is_alive(neighbor) {
					store_position.get(neighbor)
				} else {
					None
				}
			}) {
				if neighborhood.is_within_distance(
					(neighbor_position.get3() - position.get3()).magnitude_squared(),
				) {
					drawer.draw_segment(
						render::Point {
							position: position.get3(),
							color: vector![0.0, 1.0, 0.0, 1.0],
						},
						render::Point {
							position: neighbor_position.get3(),
							color: vector![0.0, 0.0, 1.0, 1.0],
						},
					);
				}
			}
		}
	}
}

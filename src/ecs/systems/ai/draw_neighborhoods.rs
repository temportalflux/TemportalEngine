use crate::{
	ecs::{
		self,
		components::{ai::steering::Neighborhood, Position2D},
		Join, NamedSystem,
	},
	render,
};
use std::sync::{Arc, RwLock};

pub struct DrawNeighborhoods {
	debug_render: Arc<RwLock<render::DebugRender>>,
}

impl DrawNeighborhoods {
	pub fn new(debug_render: Arc<RwLock<render::DebugRender>>) -> Self {
		Self { debug_render }
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
		let mut drawer = self.debug_render.write().unwrap();
		for (neighborhood, position) in (&store_neighborhood, &store_position).join() {
			for neighbor_position in neighborhood.neighbors.iter().filter_map(|neighbor_ent_id| {
				let neighbor = store_entities.entity(*neighbor_ent_id);
				if store_entities.is_alive(neighbor) {
					store_position.get(neighbor)
				} else {
					None
				}
			}) {
				// TODO: draw a line from position to neighbor position
			}
		}
	}
}

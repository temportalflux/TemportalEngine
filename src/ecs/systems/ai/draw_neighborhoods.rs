use crate::{
	ecs::{
		self,
		components::{ai::steering::Neighborhood, Position2D},
		Join, NamedSystem,
	},
	math::vector,
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
				if neighborhood
					.is_within_distance((neighbor_position.0 - position.0).magnitude_sq())
				{
					drawer.draw_segment(
						render::Point {
							position: position.0.subvec::<3>(None),
							color: vector![0.0, 1.0, 0.0, 1.0],
						},
						render::Point {
							position: neighbor_position.0.subvec::<3>(None),
							color: vector![0.0, 0.0, 1.0, 1.0],
						},
					);
				}
			}
		}
	}
}

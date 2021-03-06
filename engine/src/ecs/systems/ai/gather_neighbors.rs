use crate::ecs::{
	self,
	components::{ai::steering::Neighborhood, Orientation, Position2D},
	Join, NamedSystem,
};

pub struct GatherNeighbors {}

impl Default for GatherNeighbors {
	fn default() -> Self {
		Self {}
	}
}

impl NamedSystem for GatherNeighbors {
	fn name() -> &'static str {
		"gather_neighbors"
	}
}

impl<'a> ecs::System<'a> for GatherNeighbors {
	type SystemData = (
		ecs::Entities<'a>,
		ecs::ReadStorage<'a, Position2D>,
		ecs::ReadStorage<'a, Orientation>,
		ecs::WriteStorage<'a, Neighborhood>,
	);

	fn run(
		&mut self,
		(store_entities, store_position, store_orientation, mut store_neighborhood): Self::SystemData,
	) {
		let all_entity_positions = (&store_entities, &store_position)
			.join()
			.map(|(entity, position)| (entity, position.get3()))
			.collect::<Vec<_>>();
		for (entity, position, orientation, mut neighborhood) in (
			&store_entities,
			&store_position,
			&store_orientation,
			&mut store_neighborhood,
		)
			.join()
		{
			neighborhood.neighbors = all_entity_positions
				.iter()
				.filter_map(|(other_ent, other_pos)| {
					if *other_ent == entity {
						return None;
					}
					let self_forward_2d = orientation.up();
					let self_to_other = position.get3() - *other_pos;
					let alignment = self_forward_2d.dot(&self_to_other.normalize());
					let distance_sq = self_to_other.magnitude_squared();
					if neighborhood.is_aligned(alignment)
						&& neighborhood.is_within_distance(distance_sq)
					{
						Some(other_ent.id())
					} else {
						None
					}
				})
				.collect();
		}
	}
}

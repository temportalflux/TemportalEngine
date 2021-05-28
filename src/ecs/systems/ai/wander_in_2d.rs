use crate::{
	ecs::{
		self,
		components::{ai::Wander2D, Orientation, Position2D, Velocity2D},
		resources::DeltaTime,
		Join, NamedSystem,
	},
	math::Quaternion,
	rand::{self, Rng},
	world,
};

pub struct WanderIn2D {}

impl Default for WanderIn2D {
	fn default() -> Self {
		Self {}
	}
}

impl NamedSystem for WanderIn2D {
	fn name() -> &'static str {
		"wander_in_2d"
	}
}

impl<'a> ecs::System<'a> for WanderIn2D {
	type SystemData = (
		ecs::Read<'a, DeltaTime>,
		ecs::WriteStorage<'a, Position2D>,
		ecs::WriteStorage<'a, Orientation>,
		ecs::WriteStorage<'a, Wander2D>,
		ecs::WriteStorage<'a, Velocity2D>,
	);

	fn run(
		&mut self,
		(
			delta_time,
			mut store_position,
			mut store_orientation,
			mut store_wander,
			mut store_velocity,
		): Self::SystemData,
	) {
		let mut rng = rand::thread_rng();
		let dt = delta_time.get().as_secs_f32();
		for (position, orientation, wander, velocity) in (
			&mut store_position,
			&mut store_orientation,
			&mut store_wander,
			&mut store_velocity,
		)
			.join()
		{
			// To determine the "target" location, we need a point on the circumfrence of the circle.
			// Such a point would be determined by a rotation around the circle at a given radius.
			// We don't want the target to move too drastically, so we will use the entity's current
			// orientation + some rotational rate around the circle to determine the rotation around the cirlce.
			let random_binomial = rng.gen::<f32>() - rng.gen::<f32>();
			wander.target_orientation = Quaternion::concat(
				&wander.target_orientation,
				&Quaternion::from_axis_angle(
					world::global_forward(),
					wander.target_rate_of_change() * random_binomial,
				),
			);

			let forward2d = wander
				.entity_desired_orientation
				.rotate(&world::global_up())
				.subvec::<2>(None);

			// normalized vector from circle center in the direction of the target on the circle edge
			let target_forward = wander
				.target_orientation
				.rotate(&world::global_right())
				.subvec::<2>(None);
			// the center of the wander circle - which is a projection in front of the position of the entity
			let circle_center = *position.get() + forward2d * wander.projection_distance();
			// world position of the target on the edge of the circle
			let target_position = circle_center + target_forward * wander.circle_radius();

			// normalized vector from current position to the target on the circle edge
			let next_forward_2d = (target_position - *position.get()).normal();
			let rot_to_look_at_target =
				Quaternion::look_at_2d(&forward2d, &next_forward_2d, &world::global_forward());
			wander.entity_desired_orientation =
				orientation.get().then(&rot_to_look_at_target).normal();
			let interp_rot = Quaternion::interp_to(
				*orientation.get(),
				wander.entity_desired_orientation,
				dt,
				wander.angular_speed(),
			);

			orientation.set(interp_rot);
			velocity.set(
				interp_rot.rotate(&world::global_up()).subvec::<2>(None) * wander.linear_speed(),
			);
		}
	}
}

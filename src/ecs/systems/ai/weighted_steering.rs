use crate::{
	ecs::{
		self,
		components::{
			ai::{steering, Wander2D},
			Orientation, Position2D, Rotation, Velocity2D,
		},
		resources::DeltaTime,
		Join, NamedSystem,
	},
	math::Quaternion,
	world,
};

pub struct ApplyWeightedSteering {}

impl Default for ApplyWeightedSteering {
	fn default() -> Self {
		Self {}
	}
}

impl NamedSystem for ApplyWeightedSteering {
	fn name() -> &'static str {
		"apply_weighted_steering"
	}
}

impl ApplyWeightedSteering {
	fn get_steering_mut<T: steering::Behavior>(
		&self,
		state: &steering::State,
		weighted: &steering::Weighted,
		maybe: Option<&mut T>,
	) -> steering::Output {
		if let Some(behavior) = maybe {
			if let Some(weight) = weighted.get_weight::<T>() {
				return behavior.get_steering(state) * (*weight);
			}
		}
		steering::Output::default()
	}
}

impl<'a> ecs::System<'a> for ApplyWeightedSteering {
	type SystemData = (
		ecs::Read<'a, DeltaTime>,
		ecs::WriteStorage<'a, Position2D>,
		ecs::WriteStorage<'a, Orientation>,
		ecs::WriteStorage<'a, Velocity2D>,
		ecs::WriteStorage<'a, Rotation>,
		ecs::ReadStorage<'a, steering::Weighted>,
		ecs::WriteStorage<'a, Wander2D>,
	);

	fn run(
		&mut self,
		(
			delta_time,
			mut store_position,
			mut store_orientation,
			mut store_velocity,
			mut store_rotation,
			store_steering,
			mut store_wander,
		): Self::SystemData,
	) {
		let dt = delta_time.get().as_secs_f32();
		for (position, orientation, velocity, rotation, steering_weights, wander_maybe) in (
			&mut store_position,
			&mut store_orientation,
			&mut store_velocity,
			&mut store_rotation,
			&store_steering,
			(&mut store_wander).maybe(),
		)
			.join()
		{
			let state = steering::State {
				position: position.0.subvec::<3>(None),
				velocity: velocity.0.subvec::<3>(None),
				orientation: orientation.0,
				rotation: rotation.0,
			};
			let mut steering = steering::Output::default();

			steering += self.get_steering_mut(&state, steering_weights, wander_maybe);

			position.0 = position.0 + (velocity.0 * dt);
			orientation.0 = orientation
				.0
				.then(&Quaternion::from_axis_angle(
					world::global_forward(),
					rotation.0 * dt,
				))
				.normal();

			velocity.0 += (steering.linear_acceleration * dt).subvec::<2>(None);
			if velocity.0.magnitude_sq() > steering_weights.max_linear_speed().powi(2) {
				velocity.0 = velocity.0.normal() * steering_weights.max_linear_speed();
			}
			rotation.0 += steering.angular_acceleration * dt;
		}
	}
}

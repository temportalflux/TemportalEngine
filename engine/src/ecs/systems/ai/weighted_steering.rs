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
	math::nalgebra::UnitQuaternion,
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
				position: position.get3(),
				velocity: velocity.get3(),
				orientation: orientation.get(),
				rotation: rotation.0,
			};
			let mut steering = steering::Output::default();

			steering += self.get_steering_mut(&state, steering_weights, wander_maybe);

			position.apply(velocity.get() * dt);
			orientation.apply(UnitQuaternion::from_axis_angle(
				&world::global_forward(),
				rotation.0 * dt,
			));

			velocity.apply(steering.linear_acceleration.xy() * dt);
			velocity.apply_max_speed(steering_weights.max_linear_speed());
			rotation.0 += steering.angular_acceleration * dt;
		}
	}
}

use crate::{ecs::components::ai::steering, math};
use serde::{Deserialize, Serialize};

#[derive(Debug, Default, Clone, Copy, Serialize, Deserialize)]
pub struct Align {
	slow_radians: f32,
	max_speed: f32,
	time_to_target: f32,
}

impl Align {
	pub fn with_slow_zone(mut self, radians: f32) -> Self {
		self.slow_radians = radians;
		self
	}

	pub fn with_max_speed(mut self, radians: f32) -> Self {
		self.max_speed = radians;
		self
	}

	pub fn with_duration(mut self, seconds: f32) -> Self {
		self.time_to_target = seconds;
		self
	}

	// WILL NOT WORK IN 3D! this is only using rotation around 1 axis, not the full extent of quaternions
	pub(in crate::ecs) fn get_steering(
		&mut self,
		state: &steering::State,
		target_angle: f32,
	) -> steering::Output {
		let mut steering = steering::Output::default();

		let mut radians_to_target = target_angle - state.orientation.angle();
		radians_to_target = math::map_to(
			radians_to_target,
			-std::f32::consts::PI..std::f32::consts::PI,
			2.0 * std::f32::consts::PI,
		);
		if radians_to_target.abs() <= f32::EPSILON {
			return steering;
		}

		let speed = if radians_to_target.abs() > self.slow_radians {
			self.max_speed
		} else {
			self.max_speed * (radians_to_target.abs() / self.slow_radians)
		};
		steering.angular_acceleration = speed;
		steering.angular_acceleration *= radians_to_target / radians_to_target.abs();
		steering.angular_acceleration /= self.time_to_target;

		steering
	}
}

use crate::{
	ecs::components::ai::steering,
	math::{Quaternion, Vector},
	world,
};
use serde::{Deserialize, Serialize};

#[derive(Debug, Default, Clone, Copy, Serialize, Deserialize)]
pub struct Face {
	align: steering::Align,
}

impl Face {
	pub fn with_align(mut self, align: steering::Align) -> Self {
		self.align = align;
		self
	}

	pub(in crate::ecs) fn get_steering(
		&mut self,
		state: &steering::State,
		target_position: Vector<f32, 3>,
	) -> steering::Output {
		let forward = state.orientation.rotate(&world::global_up());
		let next_forward = (target_position - state.position).normal();
		let rot_to_look_at_target =
			Quaternion::look_at_3d(&forward, &next_forward, &world::global_forward());
		self.align
			.get_steering(state, rot_to_look_at_target.angle())
	}
}

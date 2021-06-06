use crate::{
	ecs::components::ai::steering,
	math::nalgebra::{Point3, UnitQuaternion},
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
		target_position: Point3<f32>,
	) -> steering::Output {
		let next_forward = (target_position - state.position).normalize();
		let desired_orientation =
			UnitQuaternion::face_towards(&next_forward, &world::global_forward());
		let angle = state.orientation.angle_to(&desired_orientation);
		self.align.get_steering(state, angle)
	}
}

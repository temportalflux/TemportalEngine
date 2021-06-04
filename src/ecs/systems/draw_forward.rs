use crate::{
	ecs::{
		self,
		components::{Orientation, Position2D},
		Join, NamedSystem,
	},
	math::nalgebra::vector,
	render,
};
use std::sync::{Arc, RwLock};

pub struct DrawForward {
	debug_render: Arc<RwLock<render::DebugRender>>,
}

impl DrawForward {
	pub fn new(debug_render: Arc<RwLock<render::DebugRender>>) -> Self {
		Self { debug_render }
	}
}

impl NamedSystem for DrawForward {
	fn name() -> &'static str {
		"draw_forward"
	}
}

impl<'a> ecs::System<'a> for DrawForward {
	type SystemData = (
		ecs::ReadStorage<'a, Position2D>,
		ecs::ReadStorage<'a, Orientation>,
	);

	fn run(&mut self, (store_position, store_orientation): Self::SystemData) {
		let mut drawer = self.debug_render.write().unwrap();
		for (position, orientation) in (&store_position, &store_orientation).join() {
			drawer.draw_segment(
				render::Point {
					position: position.get3(),
					color: vector![0.0, 0.0, 1.0, 1.0],
				},
				render::Point {
					position: position.get3() + orientation.up(),
					color: vector![0.0, 0.0, 1.0, 1.0],
				},
			);
		}
	}
}

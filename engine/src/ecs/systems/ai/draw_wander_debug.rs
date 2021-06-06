use crate::{
	ecs::{
		self,
		components::{ai::Wander2D, Orientation, Position2D},
		Join, NamedSystem,
	},
	math::nalgebra::vector,
	render,
};
use std::sync::{Arc, RwLock};

pub struct DrawWanderDebug {
	debug_render: Arc<RwLock<render::DebugRender>>,
}

impl DrawWanderDebug {
	pub fn new(debug_render: Arc<RwLock<render::DebugRender>>) -> Self {
		Self { debug_render }
	}
}

impl NamedSystem for DrawWanderDebug {
	fn name() -> &'static str {
		"draw_wander_debug"
	}
}

impl<'a> ecs::System<'a> for DrawWanderDebug {
	type SystemData = (
		ecs::ReadStorage<'a, Position2D>,
		ecs::ReadStorage<'a, Orientation>,
		ecs::ReadStorage<'a, Wander2D>,
	);

	fn run(&mut self, (store_position, store_orientation, store_wander): Self::SystemData) {
		let mut drawer = self.debug_render.write().unwrap();
		for (position, _, wander) in (&store_position, &store_orientation, &store_wander).join() {
			drawer.draw_segment(
				render::Point {
					position: position.get3(),
					color: vector![0.0, 0.0, 1.0, 1.0],
				},
				render::Point {
					position: wander.target_position,
					color: vector![1.0, 0.0, 1.0, 1.0],
				},
			);
		}
	}
}

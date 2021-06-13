use crate::{
	ecs::{
		self,
		components::{
			ai::{steering, Wander2D},
			BoidRender, Orientation, Position2D, Rotation, Velocity2D,
		},
		NamedSystem,
	},
	engine::{
		math::nalgebra::{UnitQuaternion, Vector2},
		rand::{self, Rng},
		world,
	},
	input, Archetype,
};

pub type MessageReceiver = std::sync::mpsc::Receiver<CreateEntityMessage>;
#[derive(Debug, Clone, Copy)]
pub struct CreateEntityMessage {
	pub count: usize,
	pub archetype: Archetype,
}

pub struct InputCreateEntity {
	receiver: MessageReceiver,
	min: Vector2<f32>,
	max: Vector2<f32>,
}

impl InputCreateEntity {
	pub fn new(receiver: MessageReceiver) -> Self {
		Self {
			receiver,
			min: Default::default(),
			max: Default::default(),
		}
	}

	pub fn with_bounds(mut self, min: Vector2<f32>, max: Vector2<f32>) -> Self {
		self.min = min;
		self.max = max;
		self
	}
}

impl NamedSystem for InputCreateEntity {
	fn name() -> &'static str {
		"input_create_entity"
	}
}

impl<'a> ecs::System<'a> for InputCreateEntity {
	type SystemData = (
		ecs::Entities<'a>,
		ecs::WriteStorage<'a, Position2D>,
		ecs::WriteStorage<'a, Velocity2D>,
		ecs::WriteStorage<'a, Orientation>,
		ecs::WriteStorage<'a, Rotation>,
		ecs::WriteStorage<'a, BoidRender>,
		ecs::WriteStorage<'a, steering::Weighted>,
		ecs::WriteStorage<'a, steering::Neighborhood>,
		ecs::WriteStorage<'a, Wander2D>,
	);
	fn run(
		&mut self,
		(
			entities,
			mut store_position,
			mut store_velocity,
			mut store_orientation,
			mut store_rotation,
			mut store_boid_render,
			mut store_weighted_steering,
			mut store_neighborhood,
			mut store_wander,
		): Self::SystemData,
	) {
		static SPAWN_VELOCITY: f32 = 0.0;

		let mut rng = rand::thread_rng();

		let mut spawn_batch = |msg: CreateEntityMessage| {
			for entity_props in (0..msg.count).map(|_| {
				let angle = rng.gen_range(0.0..360.0_f32).to_radians();
				let orientation = UnitQuaternion::from_axis_angle(&-world::global_forward(), angle);
				(
					[
						rng.gen_range(self.min.x..self.max.x),
						rng.gen_range(self.min.y..self.max.y),
					]
					.into(),
					orientation,
					(orientation * world::global_up().into_inner()) * -SPAWN_VELOCITY,
					[0.5, 0.0, 1.0, 1.0].into(),
				)
			}) {
				entities
					.build_entity()
					.with(Position2D::new(entity_props.0), &mut store_position)
					.with(Orientation::new(entity_props.1), &mut store_orientation)
					.with(Velocity2D::new(entity_props.2.xy()), &mut store_velocity)
					.with(Rotation(0.0), &mut store_rotation)
					.with(BoidRender::new(entity_props.3), &mut store_boid_render)
					.with(
						steering::Weighted::default()
							.with_max_linear(2.0)
							.with::<Wander2D>(1.0),
						&mut store_weighted_steering,
					)
					.with(
						Wander2D::default()
							.with_projection_distance(5.0)
							.with_circle_radius(4.0)
							.with_rate_of_change(70_f32.to_radians())
							.with_linear_speed(2.0)
							.with_face(
								steering::Face::default().with_align(
									steering::Align::default()
										.with_slow_zone(20_f32.to_radians())
										.with_max_speed(5_f32.to_radians())
										.with_duration(0.2),
								),
							),
						&mut store_wander,
					)
					.with(
						steering::Neighborhood::default()
							.with_alignment(0.2)
							.with_max_distance(4.0),
						&mut store_neighborhood,
					)
					.build();
			}
		};

		while let Ok(msg) = self.receiver.try_recv() {
			spawn_batch(msg);
		}

		if let Some(action) = input::read().get_user_action(0, input::ACTION_CREATE_BOID) {
			if action.on_button_pressed() {
				spawn_batch(CreateEntityMessage {
					count: 10,
					archetype: Archetype::Wander,
				});
			}
		}
	}
}

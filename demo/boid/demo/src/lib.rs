use anyhow::Result;
pub use engine;
use engine::{math::nalgebra, Application};
use std::sync::{Arc, RwLock};

#[path = "graphics/mod.rs"]
pub mod graphics;

#[path = "ecs/mod.rs"]
pub mod ecs;

#[path = "ui/mod.rs"]
pub mod ui;

#[path = "input.rs"]
pub mod input;

#[path = "game_context.rs"]
mod game_context;
pub use game_context::*;

#[path = "archetype.rs"]
mod archetype;
pub use archetype::*;

pub struct BoidDemo();
impl Application for BoidDemo {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn run() -> Result<()> {
	engine::logging::init(&engine::logging::default_path(BoidDemo::name(), None))?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;
	let arc_user = input::init();
	let (
		action_create_boid,
		action_destroy_boid,
		action_select_prev,
		action_select_next,
		action_select_none,
	) = {
		let user = arc_user.read().unwrap();
		(
			user.get_action(input::ACTION_CREATE_BOID).unwrap(),
			user.get_action(input::ACTION_DESTROY_BOID).unwrap(),
			user.get_action(input::ACTION_SELECT_PREV_BOID),
			user.get_action(input::ACTION_SELECT_NEXT_BOID),
			user.get_action(input::ACTION_SELECT_NONE_BOID),
		)
	};

	// 25px = 1m
	let pixels_per_unit: f32 = 25.0;
	let resolution = nalgebra::vector![1280.0, 720.0];
	let world_size = resolution / pixels_per_unit;
	let wrapping_world_bounds_min = -world_size / 2.0;
	let wrapping_world_bounds_max = world_size / 2.0;

	let mut ecs_context = ecs::Context::new()
		.with_component::<ecs::components::Position2D>()
		.with_component::<ecs::components::Velocity2D>()
		.with_component::<ecs::components::Orientation>()
		.with_component::<ecs::components::Rotation>()
		.with_component::<ecs::components::ai::steering::Weighted>()
		.with_component::<ecs::components::ai::steering::Neighborhood>()
		.with_component::<ecs::components::ai::Wander2D>()
		.with_system(ecs::systems::ai::ApplyWeightedSteering::default())
		.with_system(ecs::systems::ai::GatherNeighbors::default())
		.with_system(ecs::systems::MoveEntities::default())
		.with_system(
			ecs::systems::WorldBounds::default()
				.with_bounds(wrapping_world_bounds_min, wrapping_world_bounds_max),
		);

	let (create_sender, create_receiver) =
		std::sync::mpsc::sync_channel::<ecs::systems::CreateEntityMessage>(100);
	let (destroy_sender, destroy_receiver) =
		std::sync::mpsc::sync_channel::<ecs::systems::DestroyEntityMessage>(100);
	ecs_context.add_system(
		ecs::systems::InputCreateEntity::new(action_create_boid, create_receiver)
			.with_bounds(wrapping_world_bounds_min, wrapping_world_bounds_max),
	);
	ecs_context.add_system(ecs::systems::InputDestroyEntity::new(
		action_destroy_boid,
		destroy_receiver,
	));
	GameContext::write().with_senders(create_sender, destroy_sender);

	engine::window::Window::builder()
		.with_title("Boids")
		.with_size(resolution.x as f64, resolution.y as f64)
		.with_resizable(true)
		.with_application::<BoidDemo>()
		.with_clear_color([0.08, 0.08, 0.08, 1.0].into())
		.build(&mut engine)?;

	ecs_context.add_system(ecs::systems::InstanceCollector::new(
		graphics::RenderBoids::new(
			engine.render_chain().unwrap(),
			&wrapping_world_bounds_min,
			&wrapping_world_bounds_max,
		)?,
		100,
	));

	{
		let debug_render =
			engine::render::DebugRender::create(engine.render_chain().unwrap(), |debug| {
				debug.with_engine_shaders()
			})?;
		engine.add_system(debug_render.clone());
		ecs_context.add_system(ecs::systems::DrawForward::new(debug_render.clone()));
		//ecs_context.add_system(ecs::systems::ai::DrawWanderDebug::new(debug_render.clone()));
		ecs_context.add_system(
			ecs::systems::ai::DrawNeighborhoods::new(debug_render.clone()).with_select_actions(
				action_select_prev,
				action_select_next,
				action_select_none,
			),
		);
	}

	ecs_context.setup();
	let ecs_context = Arc::new(RwLock::new(ecs_context));
	engine.add_system(ecs_context);

	engine::ui::System::new(engine.render_chain().unwrap())?
		.with_engine_shaders()?
		.with_all_fonts()?
		.with_texture(&BoidDemo::get_asset_id("arrow"))?
		.with_tree_root(engine::ui::raui::make_widget!(crate::ui::root))
		.attach_system(&mut engine, None)?;

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

use crate::{
	ecs::{self, Component, Dispatcher, DispatcherBuilder, NamedSystem, System, World},
	EngineSystem,
};

pub struct Context<'a, 'b> {
	world: World,
	dispatch_builder: Option<DispatcherBuilder<'a, 'b>>,
	dispatcher: Option<Dispatcher<'a, 'b>>,
}

impl<'a, 'b> Context<'a, 'b> {
	pub fn new() -> Context<'a, 'b> {
		use ecs::WorldExt;
		Context {
			world: World::new(),
			dispatch_builder: Some(DispatcherBuilder::new()),
			dispatcher: None,
		}
	}

	pub fn with_component<T>(mut self) -> Self
	where
		T: Component,
		<T as Component>::Storage: Default,
	{
		use ecs::WorldExt;
		self.world.register::<T>();
		self
	}

	pub fn with_system<T>(mut self, system: T) -> Self
	where
		T: NamedSystem + for<'c> System<'c> + Send + 'a,
	{
		self.add_system(system);
		self
	}

	pub fn add_system<T>(&mut self, system: T)
	where
		T: NamedSystem + for<'c> System<'c> + Send + 'a,
	{
		let dependencies = T::dependencies(&system);
		self.dispatch_builder = Some(self.dispatch_builder.take().unwrap().with(
			system,
			T::name(),
			&dependencies,
		));
	}

	pub fn setup(&mut self) {
		self.world
			.insert(ecs::resources::DeltaTime(std::time::Duration::default()));
		let builder = self.dispatch_builder.take().unwrap();
		let mut dispatcher = builder.build();
		dispatcher.setup(&mut self.world);
		self.dispatcher = Some(dispatcher);
	}

	pub fn world(&mut self) -> &mut World {
		&mut self.world
	}
}

impl<'a, 'b> EngineSystem for Context<'a, 'b> {
	fn update(&mut self, delta_time: std::time::Duration) {
		use ecs::WorldExt;
		{
			let mut resource = self.world.write_resource::<ecs::resources::DeltaTime>();
			*resource = ecs::resources::DeltaTime(delta_time);
		}
		self.dispatcher.as_mut().unwrap().dispatch(&mut self.world);
		self.world.maintain();
	}
}

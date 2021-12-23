use crate::ecs::{self, NamedSystem};
use engine::{
	input,
	rand::{self, Rng},
};

pub type MessageReceiver = std::sync::mpsc::Receiver<DestroyEntityMessage>;
#[derive(Debug, Clone, Copy)]
pub struct DestroyEntityMessage {
	pub count: usize,
}

pub struct InputDestroyEntity {
	weak_action: input::action::WeakLockState,
	receiver: MessageReceiver,
}

impl InputDestroyEntity {
	pub fn new(weak_action: input::action::WeakLockState, receiver: MessageReceiver) -> Self {
		Self {
			weak_action,
			receiver,
		}
	}
}

impl NamedSystem for InputDestroyEntity {
	fn name() -> &'static str {
		"input_destroy_entity"
	}
}

impl<'a> ecs::System<'a> for InputDestroyEntity {
	type SystemData = ecs::Entities<'a>;
	fn run(&mut self, entities: Self::SystemData) {
		use ecs::Join;

		let mut rng = rand::thread_rng();
		let mut destroy_batch = |msg: DestroyEntityMessage| {
			let ent_count = entities.join().count();

			let mut ent_indices = Vec::new();
			if ent_count <= msg.count {
				ent_indices = (0..ent_count).collect();
			} else {
				while ent_indices.len() < msg.count {
					let next_index = rng.gen_range(0..ent_count);
					if !ent_indices.contains(&next_index) {
						ent_indices.push(next_index);
					}
				}
			}

			ent_indices.sort();

			let mut consumed_sized = 0;
			let mut ent_iter = entities.join();
			for ent_index in ent_indices {
				let next_index = ent_index - consumed_sized;
				consumed_sized = ent_index + 1;
				if let Some(entity) = ent_iter.nth(next_index) {
					entities.delete(entity).unwrap();
				}
			}
		};

		while let Ok(msg) = self.receiver.try_recv() {
			destroy_batch(msg);
		}

		if let Some(arc_state) = self.weak_action.upgrade() {
			let action_state = arc_state.read().unwrap();
			if action_state.on_button_pressed() {
				destroy_batch(DestroyEntityMessage { count: 10 });
			}
		}
	}
}

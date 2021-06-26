use crate::{
	asset,
	audio::{self, LOG},
	utility::AnyError,
	Engine, EngineSystem,
};
use std::{
	collections::VecDeque,
	sync::{Arc, RwLock},
};

/// A set of audio sources which should play one after the other,
/// and can be shuffled after looping through the entire sequence.
pub struct Sequence {
	current_source: Option<super::Asset>,
	queue: VecDeque<asset::Id>,
	playback_count: Option<usize>,
	asset_ids: Vec<asset::Id>,
}

impl Sequence {
	pub fn new() -> Self {
		Self {
			asset_ids: Vec::new(),
			playback_count: Some(1),
			queue: VecDeque::new(),
			current_source: None,
		}
	}

	pub fn register_to(self, engine: &mut Engine) -> Arc<RwLock<Self>> {
		let strong = Arc::new(RwLock::new(self));
		engine.add_system(&strong);
		strong
	}

	pub fn push(&mut self, source_id: asset::Id) {
		self.asset_ids.push(source_id);
	}

	pub fn with_ids(mut self, ids: Vec<asset::Id>) -> Self {
		self.asset_ids = ids;
		self
	}

	fn create_shuffled_queue(&self) -> VecDeque<asset::Id> {
		use rand::seq::SliceRandom;
		let mut rng = rand::thread_rng();
		let mut shuffled = self.asset_ids.clone();
		shuffled[..].shuffle(&mut rng);
		shuffled.into_iter().collect::<VecDeque<_>>()
	}

	fn next_source(&mut self) -> Result<Option<super::Asset>, AnyError> {
		if self.queue.is_empty() {
			let should_play_next_set = match self.playback_count {
				Some(mut loops_remaining) => {
					loops_remaining -= 1;
					loops_remaining > 0
				}
				None => true,
			};
			if should_play_next_set {
				self.queue = self.create_shuffled_queue();
			}
		}
		match self.queue.pop_front() {
			Some(id) => {
				let source = super::Asset::create(id, 1024, &mut audio::System::write().unwrap())?;
				Ok(Some(source))
			}
			None => Ok(None),
		}
	}

	fn play_next_source(&mut self) {
		use super::Source;
		match self.next_source() {
			Ok(next_source) => {
				self.current_source = next_source.map(|mut source| {
					source.play(Some(1));
					source
				});
			}
			Err(e) => {
				log::error!(target: LOG, "Failed to create next sequence source: {}", e);
			}
		}
	}
}

impl std::iter::FromIterator<asset::Id> for Sequence {
	fn from_iter<T: IntoIterator<Item = asset::Id>>(iter: T) -> Self {
		Self::new().with_ids(iter.into_iter().collect())
	}
}

impl EngineSystem for Sequence {
	fn update(&mut self, _: std::time::Duration) {
		use super::Source;
		let should_play_next = match &self.current_source {
			Some(source) => source.is_stopped(),
			None => false,
		};
		if should_play_next {
			self.play_next_source();
		}
	}
}

impl super::Source for Sequence {
	fn play(&mut self, playback_count: Option<usize>) {
		if self.current_source.is_some() {
			self.stop();
		}
		self.playback_count = playback_count;
		self.play_next_source();
	}

	fn pause(&mut self) {
		match &mut self.current_source {
			Some(source) => source.pause(),
			None => {}
		}
	}

	fn resume(&mut self) {
		match &mut self.current_source {
			Some(source) => source.resume(),
			None => {}
		}
	}

	fn stop(&mut self) {
		match &mut self.current_source {
			Some(source) => source.stop(),
			None => {}
		}
		self.current_source = None;
	}

	fn is_stopped(&self) -> bool {
		match &self.current_source {
			Some(source) => source.is_stopped(),
			None => true,
		}
	}
}

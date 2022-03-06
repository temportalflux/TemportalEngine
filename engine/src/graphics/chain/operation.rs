use crate::graphics::Chain;
use std::sync::{Arc, RwLock, Weak};

pub trait Operation {
	fn construct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	fn deconstruct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}
}

pub type ArcOperation = Arc<RwLock<dyn Operation + 'static + Send + Sync>>;
pub type WeakOperation = Weak<RwLock<dyn Operation + 'static + Send + Sync>>;

#[derive(Default)]
pub struct ProcedureOperations {
	// A list of operations to record for each phase of a render procedure.
	operations: Vec<Vec<WeakOperation>>,
}

impl ProcedureOperations {
	pub fn set_phase_count(&mut self, count: usize) {
		self.operations.clear();
		self.operations.resize(count, Vec::new());
	}

	pub fn insert(&mut self, phase_index: usize, operation: WeakOperation) {
		assert!(phase_index < self.operations.len());
	}

	pub fn iter_all(&self) -> impl std::iter::Iterator<Item = ArcOperation> + '_ {
		self.operations
			.iter()
			.map(|phase_ops| Self::iter_internal(phase_ops))
			.flatten()
	}

	pub fn iter(&self, phase_index: usize) -> impl std::iter::Iterator<Item = ArcOperation> + '_ {
		Self::iter_internal(&self.operations[phase_index])
	}

	fn iter_internal(
		ops: &Vec<WeakOperation>,
	) -> impl std::iter::Iterator<Item = ArcOperation> + '_ {
		ops.iter().filter_map(|weak| weak.upgrade())
	}

	pub fn remove_dropped_entries(&mut self) -> bool {
		let mut has_changed = false;
		for phase_ops in self.operations.iter_mut() {
			let count = phase_ops.len();
			phase_ops.retain(|weak| weak.strong_count() > 0);
			if phase_ops.len() != count {
				has_changed = true;
			}
		}
		has_changed
	}
}

use vulkan_rs::command;

use crate::graphics::Chain;
use std::sync::{Arc, RwLock, Weak};

#[derive(Copy, Clone, Debug, PartialOrd, Ord, PartialEq, Eq)]
pub enum RequiresRecording {
	NotRequired,
	CurrentFrame,
	AllFrames,
}

pub trait Operation: 'static + Send + Sync {
	/// Initialize the operation on the first possible frame.
	/// Garunteed to be called before [`construct`] and [`prepare_for_frame`].
	///
	/// This function is only called once after the operation has been added to the chain
	/// (though if it is removed/dropped and re-added, it will be called again).
	/// Use this function to create any frame-independent resources
	/// and pull relevant structural data from the chain.
	///
	/// If an operation is added to the chain after the first frame,
	/// this function will still be called before [`construct`],
	/// but the chain itself will have already been constructed.
	fn initialize(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	/// Called on the first frame after the operation is added to the chain,
	/// or any time the chain is reconstructed (i.e. chain resolution changed).
	/// Never called before the operation's [`initialize`] function.
	///
	/// Use this to create any per-frame or resolution dependent resources.
	fn construct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	/// Called when the chain gets reconstructed (e.g. chain resolution changed)
	/// to cause all frame-dependent resources to be dropped.
	/// Never called before the first call to [`construct`].
	/// This function is not called when the chain is dropped.
	///
	/// Use this to drop any frame-dependent resources (like pipelines).
	fn deconstruct(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	/// Called after all operations have been constructed, before the chain starts rendering a frame.
	/// The chain is garunteed to be constructed by the time this function is called
	/// (if the chain needs to be reconstructed, that happens before this event).
	fn prepare_for_frame(&mut self, _chain: &Chain) -> anyhow::Result<()> {
		Ok(())
	}

	/// Called when the chain is ready to submit the command buffer for a frame,
	/// but before it records the command buffer (if the buffer needs recording).
	/// Returning true from this function indicates that the operation needs the
	/// command buffer to be recorded (regardless of if it already needed recording).
	///
	/// Use this to update any immediate-mode rendering or update uniforms.
	fn prepare_for_submit(&mut self, _chain: &Chain) -> anyhow::Result<RequiresRecording> {
		Ok(RequiresRecording::NotRequired)
	}

	/// Record the operation to the command buffer for a given frame (`buffer_index`).
	fn record(
		&mut self,
		_buffer: &mut command::Buffer,
		_buffer_index: usize,
	) -> anyhow::Result<()> {
		Ok(())
	}
}

pub type ArcOperation = Arc<RwLock<dyn Operation + 'static + Send + Sync>>;
pub type WeakOperation = Weak<RwLock<dyn Operation + 'static + Send + Sync>>;

#[derive(Default)]
pub struct ProcedureOperations {
	// A list of operations to record for each phase of a render procedure.
	operations: Vec<Vec<WeakOperation>>,
	uninitialized: Vec<WeakOperation>,
}

impl ProcedureOperations {
	pub fn set_phase_count(&mut self, count: usize) {
		self.operations.clear();
		self.operations.resize(count, Vec::new());
	}

	pub fn insert(&mut self, phase_index: usize, operation: WeakOperation) {
		assert!(phase_index < self.operations.len());
		self.operations[phase_index].push(operation.clone());
		self.uninitialized.push(operation);
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

	#[profiling::function]
	pub fn remove_dropped(&mut self) -> bool {
		let mut has_changed = false;
		// Retain only the operations that have not been dropped.
		// If any have been dropped, then the chain needs to be re-recorded.
		for phase_ops in self.operations.iter_mut() {
			let count = phase_ops.len();
			phase_ops.retain(|weak| weak.strong_count() > 0);
			if phase_ops.len() != count {
				has_changed = true;
			}
		}
		has_changed
	}

	pub fn take_unintialized(&mut self) -> Vec<WeakOperation> {
		self.uninitialized.drain(..).collect()
	}

	#[profiling::function]
	pub fn initialize_new_operations(
		&self,
		uninitialized: Vec<WeakOperation>,
		chain: &Chain,
	) -> anyhow::Result<bool> {
		let mut has_changed = false;

		// Initialize any of the operations which have been added
		// since the last call, and which are still held somewhere.
		for weak in uninitialized.into_iter() {
			let arc = match weak.upgrade() {
				Some(arc) => arc,
				// operations which were added but dont actually exist anymore are inconsequential and can be ignored.
				None => continue,
			};

			// There is at least 1 new operation, so the chain needs to be re-recorded.
			has_changed = true;

			if let Ok(mut locked) = arc.write() {
				// Initialize the new operation
				locked.initialize(&chain)?;
				// and the operation was added after the first frame of the application,
				// then construct the resources for the operation as well.
				if chain.is_constructed() {
					locked.construct(&chain)?;
				}
			}; // ; here forces the write-result to be dropped before the arc
		}

		Ok(has_changed)
	}

	#[profiling::function]
	pub fn construct(&self, chain: &Chain) -> anyhow::Result<()> {
		for operation in self.iter_all() {
			if let Ok(mut locked) = operation.write() {
				locked.construct(&chain)?;
			}
		}
		Ok(())
	}

	#[profiling::function]
	pub fn deconstruct(&self, chain: &Chain) -> anyhow::Result<()> {
		for operation in self.iter_all() {
			if let Ok(mut locked) = operation.write() {
				locked.deconstruct(&chain)?;
			}
		}
		Ok(())
	}

	#[profiling::function]
	pub fn prepare_for_frame(&self, chain: &Chain) -> anyhow::Result<()> {
		for operation in self.iter_all() {
			if let Ok(mut locked) = operation.write() {
				locked.prepare_for_frame(&chain)?;
			}
		}
		Ok(())
	}

	#[profiling::function]
	pub fn prepare_for_submit(&self, chain: &Chain) -> anyhow::Result<RequiresRecording> {
		use std::cmp::Ord;
		let mut requires_recording = RequiresRecording::NotRequired;
		for operation in self.iter_all() {
			if let Ok(mut locked) = operation.write() {
				let should_record = locked.prepare_for_submit(&chain)?;
				requires_recording = requires_recording.max(should_record);
			}
		}
		Ok(requires_recording)
	}
}

use std::{
	sync::{Arc, Mutex, Weak},
	task::{Context, Poll, Waker},
};

pub struct Semaphore(Weak<()>);

impl Semaphore {
	pub fn is_complete(&self) -> bool {
		self.0.strong_count() == 0
	}
}

pub type ArctexState = Arc<Mutex<TaskState>>;
pub struct TaskState {
	is_complete: bool,
	waker: Option<Waker>,
	strong_semaphore: Arc<()>,
}
impl Default for TaskState {
	fn default() -> Self {
		Self {
			is_complete: false,
			waker: None,
			strong_semaphore: Arc::new(()),
		}
	}
}
impl TaskState {
	fn poll(&mut self, ctx: &mut Context<'_>) -> Poll<()> {
		if !self.is_complete {
			self.waker = Some(ctx.waker().clone());
			Poll::Pending
		} else {
			Poll::Ready(())
		}
	}

	pub fn mark_complete(&mut self) {
		self.is_complete = true;
		if let Some(waker) = self.waker.take() {
			waker.wake();
		}
	}
}

pub trait ScheduledTask {
	fn state(&self) -> &ArctexState;

	fn semaphore(&self) -> Semaphore {
		let state = self.state().lock().unwrap();
		Semaphore(Arc::downgrade(&state.strong_semaphore))
	}

	fn poll_state(&self, ctx: &mut Context<'_>) -> Poll<()> {
		let mut state = self.state().lock().unwrap();
		state.poll(ctx)
	}

	/// Sends the task to the engine task management,
	/// where it will run until the operation is complete,
	/// and then be dropped (thereby dropping all of its contents).
	fn send_to(self, spawner: &Arc<super::Sender>) -> Semaphore
	where
		Self: futures::future::Future<Output = ()> + 'static + Sized + Send + Sync,
	{
		let semaphore = self.semaphore();
		spawner.spawn(self);
		semaphore
	}
}

pub fn wait_for_all(semaphores: &mut Vec<Semaphore>, delay: std::time::Duration) {
	while !semaphores.is_empty() {
		std::thread::sleep(delay);
		semaphores.retain(|semaphore| !semaphore.is_complete());
	}
}

use std::{
	sync::{Arc, Mutex, Weak},
	task::{Context, Poll, Waker},
};

pub type ArctexState = Arc<Mutex<TaskState>>;
pub struct TaskState {
	is_complete: bool,
	waker: Option<Waker>,
}
impl Default for TaskState {
	fn default() -> Self {
		Self {
			is_complete: false,
			waker: None,
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

	fn poll_state(&self, ctx: &mut Context<'_>) -> Poll<()> {
		let mut state = self.state().lock().unwrap();
		state.poll(ctx)
	}

	/// Sends the task to the engine task management,
	/// where it will run until the operation is complete,
	/// and then be dropped (thereby dropping all of its contents).
	fn send_to(self, spawner: &Arc<super::Sender>) -> Weak<super::Task>
	where
		Self: futures::future::Future<Output = ()> + 'static + Sized + Send + Sync,
	{
		spawner.spawn(self)
	}
}

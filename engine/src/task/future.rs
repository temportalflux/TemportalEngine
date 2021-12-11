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
	on_complete_delegates: Vec<Box<dyn Fn() + Send + Sync>>,
}

impl Default for TaskState {
	fn default() -> Self {
		Self {
			is_complete: false,
			waker: None,
			strong_semaphore: Arc::new(()),
			on_complete_delegates: Vec::new(),
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

impl Drop for TaskState {
	fn drop(&mut self) {
		for callback in self.on_complete_delegates.iter() {
			callback();
		}
	}
}

pub trait ScheduledTask {
	fn state(&self) -> &ArctexState;

	fn add_on_complete_callback<F>(self, callback: F) -> Self
	where
		Self: 'static + Sized,
		F: Fn() + 'static + Send + Sync,
	{
		self.state()
			.lock()
			.unwrap()
			.on_complete_delegates
			.push(Box::new(callback));
		self
	}

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

	fn join(self, delay_between_queries: std::time::Duration, timeout: Option<std::time::Duration>)
	where
		Self: Sized,
	{
		let start = std::time::Instant::now();
		let mut is_complete = self.state().lock().unwrap().is_complete;
		while !is_complete {
			let runtime = std::time::Instant::now().duration_since(start);
			if let Some(timeout) = timeout {
				if runtime >= timeout {
					break;
				}
			}
			std::thread::sleep(delay_between_queries);
			is_complete = self.state().lock().unwrap().is_complete;
		}
	}
}

pub fn wait_for_all(semaphores: &mut Vec<Semaphore>, delay: std::time::Duration) {
	while !semaphores.is_empty() {
		std::thread::sleep(delay);
		semaphores.retain(|semaphore| !semaphore.is_complete());
	}
}

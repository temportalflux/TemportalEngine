use super::Task;
use futures::task;
use std::mem::MaybeUninit;
use std::sync::{
	mpsc::{Receiver, TryRecvError},
	Arc,
};

pub struct Watcher(pub(super) Receiver<Arc<Task>>);
pub static mut WATCHER: MaybeUninit<Watcher> = MaybeUninit::uninit();

impl Watcher {
	pub fn poll_until_empty(&self) {
		while self.poll() {}
	}

	/// Polls any tasks that have been send for execution,
	/// but does not block the thread if there are no tasks to execute/poll.
	pub fn poll(&self) -> bool {
		'poll_next_task: loop {
			// Consume the next task in the channel
			match self.0.try_recv() {
				Ok(task) => {
					let mut slot = task.future.lock().unwrap();
					if let Some(mut future) = slot.take() {
						// The waker for a given task will re-send the task on the channel,
						// when the `wake` is called on its context.
						let waker = task::waker_ref(&task);
						let ctx = &mut task::Context::from_waker(&*waker);
						// Poll the task, and if it is incomplete, send it back onto the channel.
						if let task::Poll::Pending = future.as_mut().poll(ctx) {
							// The item is still pending, so the task/waker should retain ownership of the future
							*slot = Some(future);
						}
					}
				}
				Err(e) => match e {
					// this is ok and expected, the loop should exit
					TryRecvError::Empty => return false,
					// this is not really expected, but it shouldnt panic or cause errors
					TryRecvError::Disconnected => {
						log::warn!("Task watcher's channel has been disconnected");
						break 'poll_next_task;
					}
				},
			}
		}
		true
	}
}

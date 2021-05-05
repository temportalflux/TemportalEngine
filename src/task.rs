use futures::{
	future::{BoxFuture, Future},
	task, FutureExt,
};
use std::sync::{
	mpsc::{self, Receiver, SyncSender, TryRecvError},
	Arc, Mutex,
};

pub fn create_system() -> (Arc<Spawner>, Arc<Watcher>) {
	let (sender, receiver) = mpsc::sync_channel(10_000);
	(
		Arc::new(Spawner { sender }),
		Arc::new(Watcher {
			task_channel: receiver,
		}),
	)
}

struct Task {
	future: Mutex<Option<BoxFuture<'static, ()>>>,
	sender: SyncSender<Arc<Task>>,
}

impl task::ArcWake for Task {
	fn wake_by_ref(arc_self: &Arc<Self>) {
		let cloned = arc_self.clone();
		arc_self.sender.send(cloned).expect("too many tasks queued");
	}
}

pub struct Spawner {
	sender: SyncSender<Arc<Task>>,
}

impl Spawner {
	pub fn spawn<T>(&self, future: T)
	where
		T: Future<Output = ()> + 'static + Send,
	{
		let future = future.boxed();
		let task = Arc::new(Task {
			future: Mutex::new(Some(future)),
			sender: self.sender.clone(),
		});
		if let Err(e) = self.sender.try_send(task) {
			log::error!("Failed to spawn task: {:?}", e);
		}
	}
}

pub struct Watcher {
	task_channel: Receiver<Arc<Task>>,
}

impl Watcher {
	/// Polls any tasks that have been send for execution,
	/// but does not block the thread if there are no tasks to execute/poll.
	pub fn poll(&mut self) {
		'poll_next_task: loop {
			// Consume the next task in the channel
			match self.task_channel.try_recv() {
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
					TryRecvError::Empty => break 'poll_next_task,
					// this is not really expected, but it shouldnt panic or cause errors
					TryRecvError::Disconnected => {
						log::warn!("Task watcher's channel has been disconnected");
						break 'poll_next_task;
					}
				},
			}
		}
	}
}

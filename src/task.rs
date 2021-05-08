use futures::{
	future::{BoxFuture, Future},
	task, FutureExt,
};
use std::sync::{
	mpsc::{self, Receiver, SyncSender, TryRecvError},
	Arc, Mutex,
};

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

pub struct Watcher(Receiver<Arc<Task>>);
pub struct Sender(SyncSender<Arc<Task>>);
static mut WATCHER: std::mem::MaybeUninit<Watcher> = std::mem::MaybeUninit::uninit();
static mut SENDER: std::mem::MaybeUninit<Arc<Sender>> = std::mem::MaybeUninit::uninit();

pub fn initialize_system() -> &'static Watcher {
	static mut ONCE: std::sync::Once = std::sync::Once::new();
	unsafe {
		ONCE.call_once(|| {
			let (sender, receiver) = mpsc::sync_channel(10_000);
			WATCHER.as_mut_ptr().write(Watcher(receiver));
			SENDER.as_mut_ptr().write(Arc::new(Sender(sender)));
		});
	}
	unsafe { &*WATCHER.as_ptr() }
}

pub fn watcher() -> &'static Watcher {
	unsafe { &*WATCHER.as_ptr() }
}

pub fn sender() -> &'static Arc<Sender> {
	unsafe { &*SENDER.as_ptr() }
}

impl Sender {
	pub fn spawn<T>(&self, future: T)
	where
		T: Future<Output = ()> + 'static + Send,
	{
		let future = future.boxed();
		let task = Arc::new(Task {
			future: Mutex::new(Some(future)),
			sender: self.0.clone(),
		});
		if let Err(e) = self.0.try_send(task) {
			log::error!("Failed to spawn task: {:?}", e);
		}
	}
}

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

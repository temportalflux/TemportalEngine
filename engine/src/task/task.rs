use futures::{future::BoxFuture, task};
use std::sync::{mpsc::SyncSender, Arc, Mutex};

pub struct Task {
	pub(super) future: Mutex<Option<BoxFuture<'static, ()>>>,
	pub(super) sender: SyncSender<Arc<Task>>,
}

impl task::ArcWake for Task {
	fn wake_by_ref(arc_self: &Arc<Self>) {
		let cloned = arc_self.clone();
		arc_self.sender.send(cloned).expect("too many tasks queued");
	}
}

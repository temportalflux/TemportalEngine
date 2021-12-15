use super::Task;
use futures::{future::Future, FutureExt};
use std::mem::MaybeUninit;
use std::sync::{mpsc::SyncSender, Arc, Mutex};

pub struct Sender(pub(super) SyncSender<Arc<Task>>);
pub static mut SENDER: MaybeUninit<Arc<Sender>> = MaybeUninit::uninit();

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
use crate::utility::Result;
use crossbeam_channel;
use std::{
	mem::MaybeUninit,
	sync::{Arc, Once},
};

pub use tokio::task::JoinHandle;

pub fn current() -> tokio::runtime::Handle {
	tokio::runtime::Handle::current()
}

pub fn spawn<T>(target: String, future: T) -> JoinHandle<()>
where
	T: futures::future::Future<Output = Result<()>> + Send + 'static,
{
	tokio::task::spawn(async move {
		if let Err(err) = future.await {
			log::error!(target: &target, "{:?}", err);
		}
	})
}

pub fn spawn_blocking<F>(target: String, callback: F)
where
	F: FnOnce() -> Result<()> + Send + 'static,
{
	tokio::task::spawn_blocking(move || {
		if let Err(err) = callback() {
			log::error!(target: &target, "{:?}", err);
		}
	});
}

type AnyItem = Box<dyn std::any::Any + Send + Sync + 'static>;

type Sender = crossbeam_channel::Sender<AnyItem>;
static mut SENDER: MaybeUninit<Arc<Sender>> = MaybeUninit::uninit();
fn sender() -> &'static Arc<Sender> {
	unsafe { &*SENDER.as_ptr() }
}

type Receiver = crossbeam_channel::Receiver<AnyItem>;
static mut RECEIVER: MaybeUninit<Receiver> = MaybeUninit::uninit();
fn receiver() -> &'static Receiver {
	unsafe { &*RECEIVER.as_ptr() }
}

pub fn initialize_system() {
	static mut ONCE: Once = Once::new();
	unsafe {
		ONCE.call_once(|| {
			let (sender, receiver) = crossbeam_channel::unbounded();
			SENDER.as_mut_ptr().write(Arc::new(sender));
			RECEIVER.as_mut_ptr().write(receiver);
		});
	}
}

pub fn send_to_main_thread<T>(any: T) -> Result<()>
where
	T: Send + Sync + 'static,
{
	Ok(sender().try_send(Box::new(any))?)
}

// Processes all of the items sent to the main thread to be explicitly dropped on the main thread.
#[profiling::function]
pub(crate) fn poll_until_empty() {
	use crossbeam_channel::TryRecvError;
	loop {
		match receiver().try_recv() {
			Ok(_item) => {
				// item has been received on the main thread, its time to drop it
			}
			Err(TryRecvError::Empty | TryRecvError::Disconnected) => {
				break;
			}
		}
	}
}

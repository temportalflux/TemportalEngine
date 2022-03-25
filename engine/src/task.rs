use anyhow::Result;
use futures_util::future::Future;
use std::{
	mem::MaybeUninit,
	pin::Pin,
	sync::{Arc, Once},
};
use tokio::task::JoinError;

pub use tokio::task::JoinHandle;

pub mod scope;

/// The set of root async tasks that can be observed.
pub fn global_scopes() -> &'static Arc<scope::List> {
	static mut SCOPES: (MaybeUninit<Arc<scope::List>>, Once) = (MaybeUninit::uninit(), Once::new());
	unsafe {
		SCOPES.1.call_once(|| {
			SCOPES
				.0
				.as_mut_ptr()
				.write(Arc::new(scope::List::new("global".to_owned())))
		});
		&*SCOPES.0.as_ptr()
	}
}

pub type PinFutureResult<T> = PinFutureResultLifetime<'static, T>;
pub type PinFutureResultLifetime<'l, T> =
	Pin<Box<dyn Future<Output = anyhow::Result<T>> + 'l + Send>>;

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

pub async fn join_all<I, S>(i: I) -> anyhow::Result<bool>
where
	I: IntoIterator,
	I::Item: Future<Output = Result<anyhow::Result<S>, JoinError>> + Send,
{
	let mut all_ok = true;
	let results = futures::future::join_all(i).await;
	for result in results.into_iter() {
		match result {
			Ok(Ok(_s)) => {}
			// if any are errors, then return that one of the internal items failed
			_ => {
				all_ok = false;
			}
		}
	}
	Ok(all_ok)
}

pub async fn join_handles<I>(i: I) -> Result<(), Vec<anyhow::Error>>
where
	I: IntoIterator,
	I::Item: Future<Output = Result<anyhow::Result<()>, JoinError>> + Send,
{
	let results = futures::future::join_all(i).await;
	let mut errors = Vec::with_capacity(results.len());
	for join_result in results.into_iter() {
		match join_result {
			Ok(task_result) => match task_result {
				Ok(_) => {} // NO-OP
				Err(task_error) => {
					errors.push(task_error);
				}
			},
			Err(join_error) => {
				errors.push(anyhow::Error::from(join_error));
			}
		}
	}
	match errors.is_empty() {
		true => Ok(()),
		false => Err(errors),
	}
}

type AnyItem = Box<dyn std::any::Any + Send + Sync + 'static>;

type Sender = crate::channels::mpsc::Sender<AnyItem>;
static mut SENDER: MaybeUninit<Arc<Sender>> = MaybeUninit::uninit();
fn sender() -> &'static Arc<Sender> {
	unsafe { &*SENDER.as_ptr() }
}

type Receiver = crate::channels::mpsc::Receiver<AnyItem>;
static mut RECEIVER: MaybeUninit<Receiver> = MaybeUninit::uninit();
fn receiver() -> &'static Receiver {
	unsafe { &*RECEIVER.as_ptr() }
}

pub fn initialize_system() {
	static mut ONCE: Once = Once::new();
	unsafe {
		ONCE.call_once(|| {
			let (sender, receiver) = crate::channels::mpsc::unbounded();
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
	use crate::channels::mpsc::TryRecvError;
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

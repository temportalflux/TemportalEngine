mod future;
pub use future::*;
mod task;
pub use task::*;
mod sender;
pub use sender::*;
mod watcher;
pub use watcher::*;

use std::sync::{mpsc, Arc, Once};

pub fn initialize_system() -> &'static Watcher {
	static mut ONCE: Once = Once::new();
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

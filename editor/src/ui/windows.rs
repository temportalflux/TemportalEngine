use engine::ui::egui::window::{List, Request, Sender};
use std::{
	mem::MaybeUninit,
	sync::{Arc, Once, RwLock},
};

mod asset_browser;
pub use asset_browser::*;
mod simulation;
pub use simulation::*;

fn global_list_data() -> &'static (Arc<RwLock<List>>, Arc<Sender>) {
	static mut DATA: (MaybeUninit<(Arc<RwLock<List>>, Arc<Sender>)>, Once) =
		(MaybeUninit::uninit(), Once::new());
	unsafe {
		DATA.1.call_once(|| {
			let (sender, receiver) = Request::channel();
			let list = Arc::new(RwLock::new(List::new(Some(receiver))));
			DATA.0.as_mut_ptr().write((list, Arc::new(sender)))
		});
		&*DATA.0.as_ptr()
	}
}

pub fn global_list() -> &'static Arc<RwLock<List>> {
	&global_list_data().0
}

pub fn global_sender() -> &'static Arc<Sender> {
	&global_list_data().1
}

use crate::ui::{
	oui::{widget::LockedReadWidget, AsRAUI},
	raui::*,
};

pub trait Slot {
	fn get<'a>(&'a self) -> LockedReadWidget<'a>;
}

impl<T> AsRAUI for T
where
	T: Slot,
{
	fn as_raui(&self) -> WidgetComponent {
		match self.get().ok() {
			Some(guard) => guard.as_raui(),
			None => unimplemented!(),
		}
	}
}

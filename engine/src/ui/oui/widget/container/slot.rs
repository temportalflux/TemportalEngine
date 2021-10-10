use crate::ui::{
	oui::{
		widget::{ArcLockWidget, LockedReadWidget},
		AsRAUI,
	},
	raui::*,
};

pub trait Slot {
	fn get<'a>(&'a self) -> LockedReadWidget<'a>;

	fn get_as_raui(&self) -> WidgetComponent {
		match self.get().ok() {
			Some(guard) => guard.as_raui(),
			None => unimplemented!(),
		}
	}
}

pub struct GenericSlot {
	content: ArcLockWidget,
}

impl From<ArcLockWidget> for GenericSlot {
	fn from(content: ArcLockWidget) -> Self {
		Self { content }
	}
}

impl Slot for GenericSlot {
	fn get<'a>(&'a self) -> LockedReadWidget<'a> {
		self.content.read()
	}
}

impl AsRAUI for GenericSlot {
	fn as_raui(&self) -> WidgetComponent {
		self.get_as_raui()
	}
}

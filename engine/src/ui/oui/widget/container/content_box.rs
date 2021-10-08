use crate::ui::{
	oui::{
		widget::{container, ArcLockWidget, LockedReadWidget, Widget},
		AsRAUI,
	},
	raui::*,
};

pub struct Container {
	slots: container::Container<Slot>,
}

pub struct Slot {
	content: ArcLockWidget,
}

impl container::Slot for Slot {
	fn get<'a>(&'a self) -> LockedReadWidget<'a> {
		self.content.read()
	}
}

impl Container {
	pub fn new() -> Self {
		Self {
			slots: container::Container::new(),
		}
	}
}

impl Widget for Container {}

impl AsRAUI for Container {
	fn as_raui(&self) -> WidgetComponent {
		self.slots
			.iter()
			.fold(make_widget!(nav_content_box), |content_box, slot| {
				content_box.listed_slot(slot.as_raui())
			})
	}
}

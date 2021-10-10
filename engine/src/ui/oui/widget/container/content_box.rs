use crate::ui::{
	oui::{
		widget::{container, ArcLockWidget, LockedReadWidget, Widget},
		AsRAUI,
	},
	raui::*,
};
use std::sync::{Arc, RwLock};

pub struct Container {
	slots: container::Container<Slot>,
}

pub struct Slot {
	content: ArcLockWidget,
	layout: ContentBoxItemLayout,
}

impl<T> From<Arc<RwLock<T>>> for Slot
where
	T: 'static + Widget + Send + Sync,
{
	fn from(content: Arc<RwLock<T>>) -> Self {
		Self {
			content,
			layout: ContentBoxItemLayout::default(),
		}
	}
}

impl Slot {
	pub fn with_layout(mut self, layout: ContentBoxItemLayout) -> Self {
		self.layout = layout;
		self
	}
}

impl container::Slot for Slot {
	fn get<'a>(&'a self) -> LockedReadWidget<'a> {
		self.content.read()
	}
}

impl AsRAUI for Slot {
	fn as_raui(&self) -> WidgetComponent {
		use container::Slot;
		self.get_as_raui().with_props(self.layout.clone())
	}
}

impl Container {
	pub fn new() -> Self {
		Self {
			slots: container::Container::new(),
		}
	}

	pub fn with_slot<T>(mut self, slot_or_widget: T) -> Self
	where
		T: Into<Slot>,
	{
		self.slots.push(slot_or_widget.into());
		self
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

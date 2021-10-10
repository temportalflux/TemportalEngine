use crate::ui::{
	oui::{
		widget::{container::GenericSlot, Widget},
		AsRAUI,
	},
	raui::*,
};

pub struct SizeBox {
	props: SizeBoxProps,
	navicable: bool,
	content_slot: Option<GenericSlot>,
}

impl SizeBox {
	pub fn new() -> Self {
		Self {
			props: SizeBoxProps::default(),
			navicable: false,
			content_slot: None,
		}
	}

	pub fn with_navicability(mut self) -> Self {
		self.navicable = true;
		self
	}

	pub fn with_width(mut self, value: SizeBoxSizeValue) -> Self {
		self.props.width = value;
		self
	}

	pub fn with_height(mut self, value: SizeBoxSizeValue) -> Self {
		self.props.height = value;
		self
	}

	pub fn with_content<T>(mut self, slot_or_widget: T) -> Self
	where
		T: Into<GenericSlot>,
	{
		self.content_slot = Some(slot_or_widget.into());
		self
	}
}

impl Widget for SizeBox {}

impl AsRAUI for SizeBox {
	fn as_raui(&self) -> WidgetComponent {
		let mut widget = make_widget!(size_box).with_props(self.props.clone());
		if self.navicable {
			widget = widget.with_props(NavItemActive);
		}
		if let Some(slot) = &self.content_slot {
			widget = widget.named_slot("content", slot.as_raui());
		}
		widget
	}
}

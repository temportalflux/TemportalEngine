use crate::ui::{
	oui::{
		widget::{container::GenericSlot, Widget},
		AsRAUI,
	},
	raui::*,
};
use std::sync::{Arc, RwLock};

pub struct Delegate {
	callbacks: Vec<Box<dyn Fn()>>,
}

impl Delegate {
	pub fn new() -> Self {
		Self {
			callbacks: Vec::new(),
		}
	}

	pub fn add_closure<F>(&mut self, callback: F)
	where
		F: 'static + Fn(),
	{
		self.callbacks.push(Box::new(callback))
	}

	pub fn add_object<T, F>(&mut self, object: Arc<RwLock<T>>, callback: F)
	where
		T: 'static,
		F: 'static + Fn(&mut T),
	{
		self.callbacks.push(Box::new(move || {
			if let Ok(mut guard) = object.write() {
				callback(&mut *guard);
			}
		}));
	}
}

pub struct Button {
	navicable: bool,
	content_slot: Option<GenericSlot>,
	pub on_interact: Arc<RwLock<Delegate>>,
}

impl Button {
	pub fn new() -> Self {
		Self {
			navicable: false,
			content_slot: None,
			on_interact: Arc::new(RwLock::new(Delegate::new())),
		}
	}

	pub fn with_navicability(mut self) -> Self {
		self.navicable = true;
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

impl Widget for Button {}

impl AsRAUI for Button {
	fn as_raui(&self) -> WidgetComponent {
		let mut widget = make_widget!(widget);
		if self.navicable {
			widget = widget.with_props(NavItemActive);
		}
		if let Some(slot) = &self.content_slot {
			widget = widget.named_slot("content", slot.as_raui());
		}
		widget
	}
}

// use_button_notified_state - exposes `ButtonProps` as a valid state for this widget, which contains the stateful behavior data
#[pre_hooks(use_button_notified_state)]
pub fn widget(mut ctx: WidgetContext) -> WidgetNode {
	ctx.life_cycle.change(|ctx| {
		for msg in ctx.messenger.messages {
			if let Some(msg) = msg.as_any().downcast_ref::<ButtonNotifyMessage>() {
				if msg.trigger_start() {}
			}
		}
	});
	WidgetNode::Component(
		make_widget!(button).with_props(ButtonNotifyProps(ctx.id.to_owned().into())),
	)
}

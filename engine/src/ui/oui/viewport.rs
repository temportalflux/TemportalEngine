use super::{
	super::{raui::*, ContextContainer},
	widget::ArcLockWidget,
};
use std::sync::{Arc, RwLock};

pub struct Viewport {
	root_widget: Option<ArcLockWidget>,
}

impl Viewport {
	pub fn new() -> Self {
		Self { root_widget: None }
	}

	pub fn arclocked(self) -> Arc<RwLock<Self>> {
		Arc::new(RwLock::new(self))
	}

	pub fn with_root(mut self, widget: ArcLockWidget) -> Self {
		self.set_root(widget);
		self
	}

	pub fn set_root(&mut self, widget: ArcLockWidget) {
		self.root_widget = Some(widget);
	}

	pub fn take_root(&mut self) -> Option<ArcLockWidget> {
		self.root_widget.take()
	}
}

pub fn read_root_widget(ctx: &WidgetContext) -> Option<ArcLockWidget> {
	if let Some(ctx_container) = ctx.process_context.get::<ContextContainer>() {
		if let Some(guard) = ctx_container.get::<Viewport>() {
			return guard.root_widget.clone();
		}
	}
	None
}

pub fn widget(ctx: WidgetContext) -> WidgetNode {
	if let Some(root) = read_root_widget(&ctx) {
		if let Ok(guard) = root.read() {
			return WidgetNode::Component(guard.as_raui());
		}
	}
	WidgetNode::None
}

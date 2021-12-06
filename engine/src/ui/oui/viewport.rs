use super::{
	super::{raui::*, ContextContainer},
	widget::ArcLockWidget,
};
use std::sync::{Arc, RwLock};

pub trait Viewport {
	fn get_root(&self) -> &Option<ArcLockWidget>;
}

pub struct DefaultViewport {
	root_widget: Option<ArcLockWidget>,
}

impl DefaultViewport {
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

impl Viewport for DefaultViewport {
	fn get_root(&self) -> &Option<ArcLockWidget> {
		&self.root_widget
	}
}

pub fn read_root_widget<TViewport>(ctx: &WidgetContext) -> Option<ArcLockWidget>
where
	TViewport: 'static + Viewport,
{
	if let Some(ctx_container) = ctx.process_context.get::<ContextContainer>() {
		if let Some(guard) = ctx_container.get::<TViewport>() {
			return (*guard).get_root().clone();
		}
	}
	None
}

pub fn widget<TViewport>(ctx: WidgetContext) -> WidgetNode
where
	TViewport: 'static + Viewport,
{
	if let Some(root) = read_root_widget::<TViewport>(&ctx) {
		if let Ok(guard) = root.read() {
			return WidgetNode::Component(guard.as_raui());
		}
	}
	WidgetNode::None
}

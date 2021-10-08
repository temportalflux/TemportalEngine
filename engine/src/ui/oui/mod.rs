pub mod viewport;
pub mod widget;

pub use widget::Widget;

pub trait AsRAUI {
	fn as_raui(&self) -> crate::ui::raui::WidgetComponent;
}

use crate::ui;

pub struct UIFont(&'static str);

impl UIFont {
	pub const fn new(path: &'static str) -> Self {
		Self(path)
	}

	pub fn at_size(&self, size: f32) -> ui::raui::TextBoxFont {
		ui::raui::TextBoxFont {
			name: self.0.to_owned(),
			size,
		}
	}

	pub fn to_owned(&self) -> String {
		self.0.to_owned()
	}
}

pub mod font {
	pub mod unispace {
		use super::super::UIFont;
		pub static REGULAR: UIFont = UIFont("font/unispace/regular");
		pub static BOLD: &'static str = "font/unispace/bold";
		pub static ITALIC: &'static str = "font/unispace/italic";
		pub static BOLD_ITALIC: &'static str = "font/unispace/bold-italic";
	}
}

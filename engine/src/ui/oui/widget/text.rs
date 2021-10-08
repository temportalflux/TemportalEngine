use crate::ui::{oui::*, raui::*};

pub struct Text {
	props: TextBoxProps,
}

impl Text {
	pub fn new() -> Self {
		Self {
			props: TextBoxProps {
				text: String::new(),
				font: TextBoxFont {
					name: String::new(),
					size: 10.0,
				},
				color: Color {
					r: 1.0,
					g: 1.0,
					b: 1.0,
					a: 1.0,
				},
				width: TextBoxSizeValue::Fill,  // or Exact(f32)
				height: TextBoxSizeValue::Fill, // or Exact(f32)
				horizontal_align: TextBoxHorizontalAlign::Left,
				vertical_align: TextBoxVerticalAlign::Top,
				direction: TextBoxDirection::HorizontalLeftToRight,
				transform: Transform::default(),
			},
		}
	}
}

impl Text {
	pub fn with_text(mut self, value: String) -> Self {
		self.set_text(value);
		self
	}

	pub fn set_text(&mut self, value: String) {
		self.props.text = value;
	}

	pub fn with_font(mut self, value: TextBoxFont) -> Self {
		self.set_font(value);
		self
	}

	pub fn set_font(&mut self, value: TextBoxFont) {
		self.props.font = value;
	}

	pub fn with_color(mut self, value: Color) -> Self {
		self.set_color(value);
		self
	}

	pub fn set_color(&mut self, value: Color) {
		self.props.color = value;
	}

	pub fn with_width(mut self, value: TextBoxSizeValue) -> Self {
		self.set_width(value);
		self
	}

	pub fn set_width(&mut self, value: TextBoxSizeValue) {
		self.props.width = value;
	}

	pub fn with_height(mut self, value: TextBoxSizeValue) -> Self {
		self.set_height(value);
		self
	}

	pub fn set_height(&mut self, value: TextBoxSizeValue) {
		self.props.height = value;
	}

	pub fn with_align_horizontal(mut self, value: TextBoxHorizontalAlign) -> Self {
		self.set_align_horizontal(value);
		self
	}

	pub fn set_align_horizontal(&mut self, value: TextBoxHorizontalAlign) {
		self.props.horizontal_align = value;
	}

	pub fn with_align_vertical(mut self, value: TextBoxVerticalAlign) -> Self {
		self.set_align_vertical(value);
		self
	}

	pub fn set_align_vertical(&mut self, value: TextBoxVerticalAlign) {
		self.props.vertical_align = value;
	}

	pub fn with_direction(mut self, value: TextBoxDirection) -> Self {
		self.set_direction(value);
		self
	}

	pub fn set_direction(&mut self, value: TextBoxDirection) {
		self.props.direction = value;
	}
}

impl Widget for Text {}

impl AsRAUI for Text {
	fn as_raui(&self) -> WidgetComponent {
		make_widget!(text_box).with_props(self.props.clone())
	}
}

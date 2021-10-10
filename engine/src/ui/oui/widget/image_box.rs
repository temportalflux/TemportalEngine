use crate::ui::{oui::*, raui::*};

pub struct ImageBox {
	props: ImageBoxProps,
}

impl ImageBox {
	pub fn new() -> Self {
		Self {
			props: ImageBoxProps::default(),
		}
	}

	pub fn with_width(mut self, value: ImageBoxSizeValue) -> Self {
		self.props.width = value;
		self
	}

	pub fn with_height(mut self, value: ImageBoxSizeValue) -> Self {
		self.props.height = value;
		self
	}

	pub fn with_texture(mut self, id: String) -> Self {
		self.props.material = ImageBoxMaterial::Image(ImageBoxImage {
			id,
			..Default::default()
		});
		self
	}
}

impl Widget for ImageBox {}

impl AsRAUI for ImageBox {
	fn as_raui(&self) -> WidgetComponent {
		make_widget!(image_box).with_props(self.props.clone())
	}
}

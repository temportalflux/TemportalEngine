use crate::ui::text;

#[derive(Debug)]
pub enum Error {
	InvalidFont(text::font::Id),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::InvalidFont(ref font_id) => write!(f, "Unregistered font id: {}", font_id),
		}
	}
}

impl std::error::Error for Error {}

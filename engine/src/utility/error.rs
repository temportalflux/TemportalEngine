use crate::graphics::{device::physical, utility};

#[derive(Debug)]
pub enum Error {
	FailedToFindPhysicalDevice(Option<physical::Constraint>),
	Graphics(utility::Error),
	GraphicsBufferWrite(std::io::Error),
	UI(crate::ui::core::Error),
}

pub type Result<T> = std::result::Result<T, Error>;
pub type AnyError = Box<dyn std::error::Error>;
pub type VoidResult = std::result::Result<(), AnyError>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::FailedToFindPhysicalDevice(ref constraint) => match constraint {
				Some(constraint) => write!(
					f,
					"Failed to find physical device due to constraint {:?}",
					constraint
				),
				None => write!(f, "Failed to find any physical devices, do you have a GPU?"),
			},
			Error::Graphics(ref graphics_error) => graphics_error.fmt(f),
			Error::GraphicsBufferWrite(ref io_error) => io_error.fmt(f),
			Error::UI(ref ui_error) => ui_error.fmt(f),
		}
	}
}

impl std::error::Error for Error {}

impl From<utility::Error> for Error {
	fn from(err: utility::Error) -> Error {
		Error::Graphics(err)
	}
}

impl From<crate::ui::core::Error> for Error {
	fn from(err: crate::ui::core::Error) -> Error {
		Error::UI(err)
	}
}

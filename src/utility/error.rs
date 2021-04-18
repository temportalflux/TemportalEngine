use temportal_graphics::utility;

#[derive(Debug)]
pub enum Error {
	Sdl(String),
	SdlWindow(sdl2::video::WindowBuildError),
	Graphics(utility::Error),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::Sdl(ref msg) => write!(f, "Encountered SDL2 error: {}", msg),
			Error::SdlWindow(ref e) => e.fmt(f),
			Error::Graphics(ref graphics_error) => graphics_error.fmt(f),
		}
	}
}

impl std::error::Error for Error {}

pub fn as_sdl_error<T>(res: std::result::Result<T, String>) -> Result<T> {
	match res {
		Ok(v) => Ok(v),
		Err(e) => Err(Error::Sdl(e)),
	}
}

pub fn as_graphics_error<T>(res: std::result::Result<T, utility::Error>) -> Result<T> {
	match res {
		Ok(v) => Ok(v),
		Err(e) => Err(Error::Graphics(e)),
	}
}

pub fn as_window_error<T>(res: std::result::Result<T, sdl2::video::WindowBuildError>) -> Result<T> {
	match res {
		Ok(v) => Ok(v),
		Err(e) => Err(Error::SdlWindow(e)),
	}
}

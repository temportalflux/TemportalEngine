#[derive(Debug)]
pub enum Error {
	NoOutputDevice(),
	FailedToConfigureOutput(cpal::DefaultStreamConfigError),
	FailedToBuildStream(cpal::BuildStreamError),
	FailedToStartStream(cpal::PlayStreamError),
	DecodeVorbis(lewton::VorbisError),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::NoOutputDevice() => write!(f, "No available output device"),
			Error::FailedToConfigureOutput(ref err) => {
				write!(f, "Failed to configure cpal output stream: {}", err)
			}
			Error::FailedToBuildStream(ref err) => {
				write!(f, "Failed to build cpal output stream: {}", err)
			}
			Error::FailedToStartStream(ref err) => {
				write!(f, "Failed to start cpal output stream: {}", err)
			}
			Error::DecodeVorbis(ref err) => {
				write!(f, "Failed to decode vorbis: {}", err)
			}
		}
	}
}

impl std::error::Error for Error {}

impl From<cpal::DefaultStreamConfigError> for Error {
	fn from(err: cpal::DefaultStreamConfigError) -> Error {
		Error::FailedToConfigureOutput(err)
	}
}

impl From<cpal::BuildStreamError> for Error {
	fn from(err: cpal::BuildStreamError) -> Error {
		Error::FailedToBuildStream(err)
	}
}

impl From<cpal::PlayStreamError> for Error {
	fn from(err: cpal::PlayStreamError) -> Error {
		Error::FailedToStartStream(err)
	}
}

impl From<lewton::VorbisError> for Error {
	fn from(err: lewton::VorbisError) -> Error {
		Error::DecodeVorbis(err)
	}
}

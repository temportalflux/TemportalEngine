use super::decoder;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum SourceKind {
	MP3,
	WAV,
	Vorbis,
}

impl SourceKind {
	pub fn extension(&self) -> &'static str {
		match *self {
			Self::MP3 => "mp3",
			Self::WAV => "wav",
			Self::Vorbis => "ogg",
		}
	}

	pub fn create_decoder(
		&self,
		cursor: std::io::Cursor<Vec<u8>>,
	) -> Result<Box<dyn decoder::Decoder>, super::Error> {
		match *self {
			Self::Vorbis => Ok(Box::new(decoder::Vorbis::new(cursor)?)),
			_ => unimplemented!("Not yet implemented"),
		}
	}
}

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

	pub fn decode(
		&self,
		cursor: std::io::Cursor<Vec<u8>>,
	) -> Result<(u32, Vec<[oddio::Sample; 2]>), super::Error> {
		use decoder::Decoder;
		match *self {
			Self::Vorbis => decoder::Vorbis::decode(cursor),
			_ => unimplemented!("Not yet implemented"),
		}
	}
}

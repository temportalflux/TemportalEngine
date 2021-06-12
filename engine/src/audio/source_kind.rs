use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum SourceKind {
	MP3,
	WAV,
	Vorbis,
	Flac,
}

impl SourceKind {
	pub fn extension(&self) -> &'static str {
		match *self {
			SourceKind::MP3 => "mp3",
			SourceKind::WAV => "wav",
			SourceKind::Vorbis => "ogg",
			SourceKind::Flac => "flac",
		}
	}
}

mod vorbis;
pub use vorbis::*;

pub trait Decoder {
	fn decode(
		cursor: std::io::Cursor<Vec<u8>>,
	) -> Result<(u32, Vec<[oddio::Sample; 2]>), crate::audio::Error>;
}

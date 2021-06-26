use super::{resample, Decoder};
use lewton::inside_ogg::OggStreamReader;

pub struct Vorbis {
	current_frame: Vec<i16>,
	reader: OggStreamReader<std::io::Cursor<Vec<u8>>>,
	channel_count: usize,
	sample_rate: u32,
}

impl Vorbis {
	pub fn new(cursor: std::io::Cursor<Vec<u8>>) -> Result<Self, crate::audio::Error> {
		let reader = OggStreamReader::new(cursor)?;
		Ok(Self {
			sample_rate: reader.ident_hdr.audio_sample_rate,
			channel_count: reader.ident_hdr.audio_channels as usize,
			reader,
			current_frame: Vec::new(),
		})
	}
}

impl Decoder for Vorbis {
	fn sample_rate(&self) -> u32 {
		self.sample_rate
	}

	fn channel_count(&self) -> usize {
		self.channel_count
	}
}

impl std::iter::Iterator for Vorbis {
	type Item = oddio::Sample;
	fn next(&mut self) -> Option<Self::Item> {
		while self.current_frame.is_empty() {
			match self.reader.read_dec_packet_itl() {
				// possibly an empty packet, or actual data
				Ok(Some(data)) => self.current_frame = data,
				// end of stream
				Ok(None) => return None,
				// actual error case
				Err(_) => return None,
			}
		}
		Some(resample(self.current_frame.remove(0)))
	}
}

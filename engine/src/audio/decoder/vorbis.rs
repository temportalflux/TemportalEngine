use super::{super::LOG, Decoder};
use lewton::inside_ogg::OggStreamReader;

pub struct Vorbis {
	loops_remaining: Option<usize>,
	sample_rate: u32,
	iterator: OggIterator,
	compressed: std::io::Cursor<Vec<u8>>,
}

impl Vorbis {
	pub fn new(cursor: std::io::Cursor<Vec<u8>>) -> Result<Self, crate::audio::Error> {
		let reader = OggStreamReader::new(cursor.clone())?;
		let channel_count = reader.ident_hdr.audio_channels;
		assert!(channel_count > 0);
		assert!(channel_count <= 2);
		let sample_rate = reader.ident_hdr.audio_sample_rate;
		Ok(Self {
			sample_rate,
			loops_remaining: Some(1),
			compressed: cursor,
			iterator: OggIterator::from_stream(reader),
		})
	}
}

impl Decoder for Vorbis {
	fn sample_rate(&self) -> u32 {
		self.sample_rate
	}

	fn set_loops_remaining(&mut self, loops: Option<usize>) {
		self.loops_remaining = loops;
	}

	fn next_stereo(&mut self) -> Option<[oddio::Sample; 2]> {
		let next_sample = self.iterator.next();
		// if there is a sample remaining, then return it,
		// otherwise the decoder has finished.
		if next_sample.is_some() {
			return next_sample;
		}

		let should_replay = match self.loops_remaining {
			Some(mut loops) => {
				// decrement the number of loops to play, because we just finished a loop
				loops -= 1;
				// return true if we should play at least 1 more loop
				loops > 0
			}
			// if loops_remaining is none, then its an infinite loop
			None => true,
		};
		if should_replay {
			self.iterator = match OggStreamReader::new(self.compressed.clone()) {
				Ok(reader) => OggIterator::from_stream(reader),
				Err(e) => {
					log::debug!(target: LOG, "Failed to replay Vorbis decoder: {}", e);
					return None;
				}
			};
			return self.next_stereo();
		}

		None
	}
}

struct OggIterator(OggStreamReader<std::io::Cursor<Vec<u8>>>, Vec<i16>);

impl OggIterator {
	fn from_stream(stream: OggStreamReader<std::io::Cursor<Vec<u8>>>) -> Self {
		Self(stream, vec![])
	}

	fn next_sample(&mut self) -> f32 {
		use cpal::Sample;
		assert!(self.1.len() > 0);
		<f32 as cpal::Sample>::from(&self.1.remove(0)).to_f32()
	}
}

impl std::iter::Iterator for OggIterator {
	type Item = [oddio::Sample; 2];
	fn next(&mut self) -> Option<Self::Item> {
		while self.1.is_empty() {
			match self.0.read_dec_packet_itl() {
				// possibly an empty packet, or actual data
				Ok(Some(data)) => self.1 = data,
				// end of stream
				Ok(None) => return None,
				// actual error case
				Err(_) => return None,
			}
		}
		return match self.0.ident_hdr.audio_channels {
			1 => {
				let sample = self.next_sample();
				Some([sample, sample])
			}
			2 => Some([self.next_sample(), self.next_sample()]),
			_ => None,
		};
	}
}

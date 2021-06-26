use super::{super::LOG, resample, Decoder};

type Mp3Decoder = minimp3::Decoder<std::io::Cursor<Vec<u8>>>;

pub struct Mpeg {
	reader: Mp3Decoder,
	current_frame: Vec<i16>,
	channel_count: usize,
	sample_rate: u32,
}

impl Mpeg {
	pub fn new(cursor: std::io::Cursor<Vec<u8>>) -> Result<Self, crate::audio::Error> {
		let mut reader = Mp3Decoder::new(cursor.clone());
		let first_frame = reader.next_frame()?;
		Ok(Self {
			sample_rate: first_frame.sample_rate as u32,
			channel_count: first_frame.channels,
			reader,
			current_frame: first_frame.data,
		})
	}
}

impl Decoder for Mpeg {
	fn sample_rate(&self) -> u32 {
		self.sample_rate
	}

	fn channel_count(&self) -> usize {
		self.channel_count
	}
}

impl std::iter::Iterator for Mpeg {
	type Item = oddio::Sample;
	fn next(&mut self) -> Option<Self::Item> {
		while self.current_frame.is_empty() {
			match self.reader.next_frame() {
				// possibly an empty packet, or actual data
				Ok(minimp3::Frame {
					data,
					channels,
					sample_rate,
					..
				}) => {
					if sample_rate as u32 != self.sample_rate() {
						log::warn!(
							target: LOG,
							"MP3 decoder encountered inconsistent sample rate. First frame was {}, but later frame is {}.",
							self.sample_rate(), sample_rate
						);
					}
					if channels != self.channel_count() {
						log::warn!(
							target: LOG,
							"MP3 decoder encountered inconsistent channel count. First frame was {}, but later frame is {}.",
							self.channel_count(), channels
						);
					}
					self.current_frame = data;
				}
				// end of stream
				Err(minimp3::Error::Eof) => return None,
				// actual error case
				Err(e) => {
					log::error!(target: LOG, "Encountered error while decoding mp3: {}", e);
					return None;
				}
			}
		}
		Some(resample(self.current_frame.remove(0)))
	}
}

use super::{
	super::{SourceKind, LOG},
	Decoder, PlaybackLoop,
};

pub struct Track {
	decoder: Option<Box<dyn Decoder>>,
	playback_counter: PlaybackLoop,
	compressed: std::io::Cursor<Vec<u8>>,
	kind: SourceKind,
}

impl Track {
	pub fn new(kind: SourceKind, cursor: std::io::Cursor<Vec<u8>>) -> Self {
		Track {
			kind,
			compressed: cursor,
			playback_counter: PlaybackLoop::new(Some(1)),
			decoder: None,
		}
	}

	pub fn sample_rate(&self) -> Result<u32, crate::audio::Error> {
		Ok(match &self.decoder {
			Some(decoder) => decoder.sample_rate(),
			None => self
				.create_decoder()
				.ok_or(crate::audio::Error::FailedToCreateDecoder(self.kind))?
				.sample_rate(),
		})
	}

	pub fn set_playback_counter(&mut self, loops_remaining: Option<usize>) {
		self.playback_counter = PlaybackLoop::new(loops_remaining);
	}

	fn next_sample(&mut self) -> Option<oddio::Sample> {
		match &mut self.decoder {
			Some(decoder) => decoder.next(),
			None => None,
		}
	}

	pub fn sample_stereo(&mut self) -> Option<[oddio::Sample; 2]> {
		if self.decoder.is_none() {
			match self.create_decoder() {
				Some(decoder) => {
					self.decoder = Some(decoder);
				}
				None => return None,
			}
		}

		let stereo_sample = match self.decoder.as_ref().unwrap().channel_count() {
			1 => self.next_sample().map(|sample| [sample, sample]),
			2 => match (self.next_sample(), self.next_sample()) {
				(Some(c1), Some(c2)) => Some([c1, c2]),
				_ => None,
			},
			_ => None,
		};

		// if there is a sample remaining, then return it,
		// otherwise the decoder has finished.
		if stereo_sample.is_some() {
			return stereo_sample;
		}

		if self.playback_counter.next_loop() {
			return self.sample_stereo();
		}

		None
	}

	fn create_decoder(&self) -> Option<Box<dyn Decoder>> {
		match self.kind.create_decoder(self.compressed.clone()) {
			Ok(decoder) => Some(decoder),
			Err(e) => {
				log::error!(target: LOG, "Failed to create decoder: {}", e);
				return None;
			}
		}
	}
}

mod mpeg;
pub use mpeg::*;

mod track;
pub use track::*;

mod vorbis;
pub use vorbis::*;

mod wav;
pub use wav::*;

pub trait Decoder: Send + Sync + std::iter::Iterator<Item = oddio::Sample> {
	fn sample_rate(&self) -> u32;
	fn channel_count(&self) -> usize;
}

pub fn resample<T>(sample: T) -> oddio::Sample
where
	T: cpal::Sample,
{
	use cpal::Sample;
	<f32 as cpal::Sample>::from(&sample).to_f32()
}

pub fn resample_24bit(sample: i32) -> oddio::Sample {
	// based on: https://github.com/RustAudio/rodio/blob/master/src/decoder/wav.rs#L170
	resample((sample >> 8) as u16)
}

pub fn resample_8bit(sample: i8) -> oddio::Sample {
	// based on: https://github.com/RustAudio/rodio/blob/master/src/decoder/wav.rs#L176
	resample(((sample as i16) * 256) as u16)
}

pub(super) struct PlaybackLoop {
	loops_remaining: Option<usize>,
}

impl PlaybackLoop {
	pub fn new(loops: Option<usize>) -> Self {
		Self {
			loops_remaining: loops,
		}
	}

	pub fn next_loop(&mut self) -> bool {
		if let Some(loops) = &mut self.loops_remaining {
			// decrement the number of loops to play, because we just finished a loop
			*loops -= 1;
		}
		self.should_continue()
	}

	pub fn should_continue(&self) -> bool {
		match self.loops_remaining {
			None => true,
			Some(count) => count > 0,
		}
	}
}

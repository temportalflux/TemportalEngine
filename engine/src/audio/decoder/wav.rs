use super::{resample, resample_24bit, resample_8bit, Decoder};

pub struct Wav {
	reader: hound::WavReader<std::io::Cursor<Vec<u8>>>,
	channel_count: usize,
	sample_rate: u32,
}

impl Wav {
	pub fn new(cursor: std::io::Cursor<Vec<u8>>) -> Result<Self, crate::audio::Error> {
		let reader = hound::WavReader::new(cursor)?;
		Ok(Self {
			sample_rate: reader.spec().sample_rate,
			channel_count: reader.spec().channels as usize,
			reader,
		})
	}

	fn sample<T>(&mut self) -> Option<hound::Result<T>>
	where
		T: hound::Sample,
	{
		self.reader.samples::<T>().next()
	}
}

impl Decoder for Wav {
	fn sample_rate(&self) -> u32 {
		self.sample_rate
	}

	fn channel_count(&self) -> usize {
		self.channel_count
	}
}

impl std::iter::Iterator for Wav {
	type Item = oddio::Sample;
	fn next(&mut self) -> Option<Self::Item> {
		use hound::SampleFormat;
		let spec = self.reader.spec();
		match (spec.sample_format, spec.bits_per_sample) {
			(SampleFormat::Float, 32) => self.sample::<f32>().map(|v| v.unwrap_or(0.0)),
			(SampleFormat::Int, 16) => self.sample::<i16>().map(|v| resample(v.unwrap_or(0))),
			(SampleFormat::Int, 24) => self.sample::<i32>().map(|v| resample_24bit(v.unwrap_or(0))),
			(SampleFormat::Int, 8) => self.sample::<i8>().map(|v| resample_8bit(v.unwrap_or(0))),
			(format, bits) => unimplemented!("Unimplemented wav spec: {}-{:?}", bits, format),
		}
	}
}

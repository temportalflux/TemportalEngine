use super::Decoder;
use lewton::inside_ogg::OggStreamReader;

pub struct Vorbis;
impl Decoder for Vorbis {
	fn decode(
		cursor: std::io::Cursor<Vec<u8>>,
	) -> Result<(u32, Vec<[oddio::Sample; 2]>), crate::audio::Error> {
		let reader = OggStreamReader::new(cursor)?;
		let channel_count = reader.ident_hdr.audio_channels;
		assert!(channel_count > 0);
		assert!(channel_count <= 2);
		let sample_rate = reader.ident_hdr.audio_sample_rate;
		Ok((
			sample_rate,
			OggIterator::from_stream(reader).collect::<Vec<_>>(),
		))
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

mod vorbis;
pub use vorbis::*;

pub trait Decoder: Send + Sync {
	fn sample_rate(&self) -> u32;
	fn set_loops_remaining(&mut self, loops: Option<usize>);
	fn next_stereo(&mut self) -> Option<[oddio::Sample; 2]>;
}

mod asset;
pub use asset::*;

mod sequence;
pub use sequence::*;

pub type AnySource = Box<dyn Source>;

pub trait Source {
	fn play(&mut self, playback_count: Option<usize>);

	fn and_play(mut self, playback_count: Option<usize>) -> Self
	where
		Self: Sized,
	{
		self.play(playback_count);
		self
	}

	fn pause(&mut self);
	fn resume(&mut self);
	fn stop(&mut self);
	fn is_stopped(&self) -> bool;
}

use crate::audio::{decoder, Sound, System, LOG};
use crate::{asset, utility::AnyError};
use oddio;
use std::sync::{Arc, Mutex, RwLock};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum DecodingState {
	Inactive,
	Decoding,
	Cancelled,
	Complete,
	Failed,
}

impl DecodingState {
	fn is_active(&self) -> bool {
		*self == DecodingState::Decoding
	}
}

type Playback = oddio::Stop<oddio::Stream<[f32; 2]>>;
type ThreadSafeStreamHandle = Arc<Mutex<oddio::Handle<Playback>>>;
pub struct Asset {
	decoding_state: Arc<RwLock<DecodingState>>,
	handle: ThreadSafeStreamHandle,
	id: asset::Id,
}

impl Asset {
	pub fn create(
		id: asset::Id,
		number_of_frames_to_buffer: usize,
		system: &mut System,
	) -> Result<Self, AnyError> {
		let stream = oddio::Stream::new(
			Self::create_decoder(&id)?.sample_rate(),
			number_of_frames_to_buffer,
		);
		// TODO: Figure out a better way to convert the signal into a control handle than starting and pausing the audio
		let mut handle = system
			.mixer_handle
			.control::<oddio::Mixer<[oddio::Sample; 2]>, _>()
			.play(stream);
		handle.control::<Playback, _>().pause();
		Ok(Self {
			id: id.clone(),
			handle: Arc::new(Mutex::new(handle)),
			decoding_state: Arc::new(RwLock::new(DecodingState::Inactive)),
		})
	}

	pub fn id(&self) -> &asset::Id {
		&self.id
	}

	fn create_decoder(id: &asset::Id) -> Result<Box<dyn decoder::Decoder>, AnyError> {
		let asset = asset::Loader::load_sync(id)?.downcast::<Sound>().unwrap();
		let binary_cursor = std::io::Cursor::new(asset.binary().clone());
		let decoder = asset.kind().create_decoder(binary_cursor)?;
		Ok(decoder)
	}

	fn start_decoding(&mut self, playback_count: Option<usize>) {
		let id = self.id().clone();
		let arc_handle = self.handle.clone();
		let decoding_state = self.decoding_state.clone();
		std::thread::spawn(move || {
			let mut decoder = match Self::create_decoder(&id) {
				Ok(decoder) => decoder,
				Err(e) => {
					log::error!(
						target: LOG,
						"Failed to create decoder for sound asset \"{}\": {}",
						id,
						e
					);
					return;
				}
			};
			decoder.set_loops_remaining(playback_count);
			*decoding_state.write().unwrap() = DecodingState::Decoding;
			let write_sample = |rw_handle: &ThreadSafeStreamHandle, stereo_sample| -> bool {
				let mut handle = match rw_handle.lock() {
					Ok(handle) => handle,
					Err(e) => {
						log::error!(
							target: LOG,
							"Failed to get the stream-handle while decoding audio for '{}'. {}",
							id,
							e
						);
						*decoding_state.write().unwrap() = DecodingState::Failed;
						return false;
					}
				};
				let mut stream_ctrl = handle.control::<oddio::Stream<[oddio::Sample; 2]>, _>();
				stream_ctrl.write(&[stereo_sample]) > 0
			};
			// Iterate until there are no more samples in the decoder
			while decoding_state.read().unwrap().is_active() {
				match decoder.next_stereo() {
					Some(stereo_sample) => {
						// Try to write the sample to the stream,
						while !write_sample(&arc_handle, stereo_sample) {
							// and if the stream is full, hang the thread until there is enough room.
						}
					}
					None => {
						log::debug!("decoding {} has completed", id);
						*decoding_state.write().unwrap() = DecodingState::Complete;
						// decoding has concluded (which implies that we are also done any playback loops),
						// so it is time to stop the audio.
						let mut handle = arc_handle.lock().unwrap();
						let playback = handle.control::<Playback, _>();
						playback.stop();
					}
				}
			}
		});
	}
}

impl super::Source for Asset {
	fn play(&mut self, playback_count: Option<usize>) {
		{
			let mut handle = self.handle.lock().unwrap();
			let playback = handle.control::<Playback, _>();
			if playback.is_paused() {
				playback.resume();
			}
		}
		if !self.decoding_state.read().unwrap().is_active() {
			self.start_decoding(playback_count);
		}
	}

	fn pause(&mut self) {
		let mut handle = self.handle.lock().unwrap();
		let playback = handle.control::<Playback, _>();
		if !playback.is_paused() && !playback.is_stopped() {
			playback.pause();
		}
	}

	fn resume(&mut self) {
		let mut handle = self.handle.lock().unwrap();
		let playback = handle.control::<Playback, _>();
		if playback.is_paused() {
			playback.resume();
		}
	}

	fn stop(&mut self) {
		let mut handle = self.handle.lock().unwrap();
		let playback = handle.control::<Playback, _>();
		if !playback.is_stopped() {
			playback.stop();
			*self.decoding_state.write().unwrap() = DecodingState::Cancelled;
		}
	}

	fn is_stopped(&self) -> bool {
		self.handle
			.lock()
			.unwrap()
			.control::<Playback, _>()
			.is_stopped()
	}
}

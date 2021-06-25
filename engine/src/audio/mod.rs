use crate::{
	asset,
	utility::{singleton::Singleton, AnyError},
};
use cpal;
use oddio;
use std::{
	collections::HashMap,
	sync::{Arc, LockResult, RwLock, RwLockWriteGuard},
};

mod sound;
pub use sound::*;

mod source_kind;
pub use source_kind::*;

mod decoder;

pub static LOG: &'static str = "audio";

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Sound>();
}

pub struct System {
	persistent_sources: HashMap<asset::Id, Source>,

	_stream: cpal::Stream,
	mixer_handle: oddio::Handle<oddio::Mixer<[oddio::Sample; 2]>>,
	_device: cpal::Device,
	_config: cpal::StreamConfig,
}

impl System {
	unsafe fn instance() -> &'static mut Singleton<Self> {
		static mut INSTANCE: Singleton<System> = Singleton::uninit();
		&mut INSTANCE
	}

	pub fn initialize() {
		// Winit and Cpal have conflicting thread models, and must be run on separate threads:
		// - https://github.com/RustAudio/cpal/pull/504
		// - https://github.com/RustAudio/cpal/pull/330
		// - https://github.com/RustAudio/rodio/issues/214
		let thread = std::thread::spawn(|| match Self::new() {
			Ok(sys) => {
				unsafe { Self::instance() }.init_with(sys);
			}
			Err(e) => {
				log::error!(target: LOG, "Failed to initialize system: {}", e);
			}
		});
		let _ = thread.join();
	}

	fn new() -> Result<Self, Error> {
		log::info!(target: LOG, "Initializing system & output stream");
		use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
		let host = cpal::default_host();
		let device = host
			.default_output_device()
			.ok_or(Error::NoOutputDevice())?;
		let sample_rate = device.default_output_config()?.sample_rate();
		let config = cpal::StreamConfig {
			channels: 2,
			sample_rate,
			buffer_size: cpal::BufferSize::Default,
		};
		let (mixer_handle, mixer) = oddio::split(oddio::Mixer::new());
		let stream = device.build_output_stream(
			&config,
			move |out_flat: &mut [oddio::Sample], _: &cpal::OutputCallbackInfo| {
				let out_stereo: &mut [[oddio::Sample; 2]] = oddio::frame_stereo(out_flat);
				oddio::run(&mixer, sample_rate.0, out_stereo);
			},
			move |err| {
				eprintln!("{}", err);
			},
		)?;
		stream.play()?;
		Ok(Self {
			_device: device,
			_config: config,
			mixer_handle,
			_stream: stream,
			persistent_sources: HashMap::new(),
		})
	}

	fn get() -> &'static RwLock<Self> {
		unsafe { Self::instance() }.get()
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}
}

impl System {
	pub fn create_sound(&mut self, id: &crate::asset::Id) -> Result<Source, AnyError> {
		Source::create(id.clone(), 1024, self)
	}

	pub fn add_persistent_source(&mut self, handle: Source) {
		self.persistent_sources.insert(handle.id().clone(), handle);
	}

	pub fn remove_persistent_source(&mut self, handle: Source) {
		self.persistent_sources.remove(handle.id());
	}

	pub fn get_persistent_source(&self, id: &asset::Id) -> Option<&Source> {
		self.persistent_sources.get(id)
	}
}

#[derive(Debug)]
pub enum Error {
	NoOutputDevice(),
	FailedToConfigureOutput(cpal::DefaultStreamConfigError),
	FailedToBuildStream(cpal::BuildStreamError),
	FailedToStartStream(cpal::PlayStreamError),
	DecodeVorbis(lewton::VorbisError),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::NoOutputDevice() => write!(f, "No available output device"),
			Error::FailedToConfigureOutput(ref err) => {
				write!(f, "Failed to configure cpal output stream: {}", err)
			}
			Error::FailedToBuildStream(ref err) => {
				write!(f, "Failed to build cpal output stream: {}", err)
			}
			Error::FailedToStartStream(ref err) => {
				write!(f, "Failed to start cpal output stream: {}", err)
			}
			Error::DecodeVorbis(ref err) => {
				write!(f, "Failed to decode vorbis: {}", err)
			}
		}
	}
}

impl std::error::Error for Error {}

impl From<cpal::DefaultStreamConfigError> for Error {
	fn from(err: cpal::DefaultStreamConfigError) -> Error {
		Error::FailedToConfigureOutput(err)
	}
}

impl From<cpal::BuildStreamError> for Error {
	fn from(err: cpal::BuildStreamError) -> Error {
		Error::FailedToBuildStream(err)
	}
}

impl From<cpal::PlayStreamError> for Error {
	fn from(err: cpal::PlayStreamError) -> Error {
		Error::FailedToStartStream(err)
	}
}

impl From<lewton::VorbisError> for Error {
	fn from(err: lewton::VorbisError) -> Error {
		Error::DecodeVorbis(err)
	}
}

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
type ThreadSafeStreamHandle = Arc<RwLock<oddio::Handle<Playback>>>;
pub struct Source {
	decoding_state: Arc<RwLock<DecodingState>>,
	handle: ThreadSafeStreamHandle,
	id: asset::Id,
}

impl Source {
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
			handle: Arc::new(RwLock::new(handle)),
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
				let mut handle = match rw_handle.write() {
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
						*decoding_state.write().unwrap() = DecodingState::Complete;
					}
				}
			}
		});
	}

	pub fn play(&mut self, playback_count: Option<usize>) {
		{
			let mut handle = self.handle.write().unwrap();
			let playback = handle.control::<Playback, _>();
			if playback.is_paused() {
				playback.resume();
			}
		}
		if !self.decoding_state.read().unwrap().is_active() {
			self.start_decoding(playback_count);
		}
	}

	pub fn pause(&mut self) {
		let mut handle = self.handle.write().unwrap();
		let playback = handle.control::<Playback, _>();
		if !playback.is_paused() && !playback.is_stopped() {
			playback.pause();
		}
	}

	pub fn stop(&mut self) {
		let mut handle = self.handle.write().unwrap();
		let playback = handle.control::<Playback, _>();
		if !playback.is_stopped() {
			playback.stop();
			*self.decoding_state.write().unwrap() = DecodingState::Cancelled;
		}
	}
}

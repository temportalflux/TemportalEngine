use crate::utility::{singleton::Singleton, AnyError};
use cpal;
use oddio;
use std::sync::{LockResult, RwLock, RwLockWriteGuard};

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
	_stream: cpal::Stream,
	mixer_handle: oddio::Handle<oddio::Mixer<[f32; 2]>>,
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
			move |out_flat: &mut [f32], _: &cpal::OutputCallbackInfo| {
				let out_stereo: &mut [[f32; 2]] = oddio::frame_stereo(out_flat);
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
		let asset = crate::asset::Loader::load_sync(id)?
			.downcast::<Sound>()
			.unwrap();
		let (sample_rate, samples) = asset
			.kind()
			.decode(std::io::Cursor::new(asset.binary().clone()))?;
		Ok(Source {
			signal: oddio::FramesSignal::from(oddio::Frames::from_slice(sample_rate, &samples[..])),
		})
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

pub struct Source {
	signal: oddio::FramesSignal<[f32; 2]>,
}

impl Source {
	pub fn play(self, system: &mut System) -> Signal {
		Signal(
			system
				.mixer_handle
				.control::<oddio::Mixer<[f32; 2]>, _>()
				.play(self.signal),
		)
	}
}

pub struct Signal(oddio::Handle<oddio::Stop<oddio::FramesSignal<[f32; 2]>>>);

use crate::utility::{singleton::Singleton, AnyError};
use cpal;
use oddio;
use std::{
	collections::HashMap,
	sync::{LockResult, RwLock, RwLockWriteGuard},
};

pub static LOG: &'static str = "audio";

mod decoder;

mod error;
pub use error::*;

mod sound;
pub use sound::*;

pub mod source;

mod source_kind;
pub use source_kind::*;

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Sound>();
}

pub struct System {
	persistent_sources: HashMap<String, source::AnySource>,

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
	pub fn create_sound(&mut self, id: &crate::asset::Id) -> Result<source::AnySource, AnyError> {
		Ok(Box::new(source::Asset::create(id.clone(), 1024, self)?))
	}

	pub fn add_persistent_source(&mut self, id: String, source: source::AnySource) {
		self.persistent_sources.insert(id, source);
	}

	pub fn remove_persistent_source(&mut self, id: &String) {
		self.persistent_sources.remove(id);
	}

	pub fn get_persistent_source(&self, id: &String) -> Option<&source::AnySource> {
		self.persistent_sources.get(id)
	}
}

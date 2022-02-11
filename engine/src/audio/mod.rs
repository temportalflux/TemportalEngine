use crate::{asset, utility::singleton::Singleton, EngineSystem};
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
	active_sources: Vec<source::AnySource>,
	named_sources: HashMap<asset::Id, Box<source::Asset>>,

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
		let _ = std::thread::Builder::new()
			.name("audio".to_owned())
			.spawn(|| {
				profiling::register_thread!();
				profiling::scope!("init");
				match Self::new() {
					Ok(sys) => {
						unsafe { Self::instance() }.init_with(sys);
					}
					Err(e) => {
						log::error!(target: LOG, "Failed to initialize system: {}", e);
					}
				}
			})
			.unwrap()
			.join();
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
			active_sources: Vec::new(),
			named_sources: HashMap::new(),
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
	fn load_source<F>(id: asset::Id, on_loaded: F)
	where
		F: Fn(&mut Self, Box<source::Asset>) + Send + 'static,
	{
		std::thread::spawn(move || {
			profiling::register_thread!("audio-load");
			profiling::scope!("audio-load");
			log::info!(target: LOG, "Loading asset {} for playing", id);
			let asset = match asset::Loader::load_sync(&id) {
				Ok(asset) => asset.downcast::<Sound>().unwrap(),
				Err(e) => {
					log::error!(target: LOG, "Failed to load asset {}: {}", id, e);
					return;
				}
			};
			log::info!(
				target: LOG,
				"Finished loading {}, creating audio source",
				id
			);
			if let Ok(mut audio_system) = Self::write() {
				let source = match source::Asset::create(id.clone(), asset, &mut audio_system) {
					Ok(source) => source,
					Err(e) => {
						log::error!(
							target: LOG,
							"Failed to create source for asset {}: {}",
							id,
							e
						);
						return;
					}
				};
				on_loaded(&mut audio_system, Box::new(source));
			}
		});
	}

	pub fn add_source(id: asset::Id) {
		Self::load_source(id.clone(), move |audio_system: &mut Self, source| {
			audio_system.named_sources.insert(id.clone(), source);
		});
	}

	pub fn get_source(&mut self, id: &asset::Id) -> Option<&mut Box<source::Asset>> {
		self.named_sources.get_mut(id)
	}

	pub fn play_until_stopped(
		&mut self,
		mut source: source::AnySource,
		playback_count: Option<usize>,
	) {
		source.play(playback_count);
		self.active_sources.push(source);
	}

	pub fn play_immediate(id: asset::Id) {
		Self::load_source(id.clone(), move |audio_system: &mut Self, source| {
			audio_system.play_until_stopped(source, Some(1));
		});
	}
}

impl EngineSystem for System {
	#[profiling::function]
	fn update(&mut self, _: std::time::Duration, _: bool) {
		self.active_sources.retain(|source| !source.is_stopped());
	}
}

use crate::utility::{singleton::Singleton, AnyError};
use rodio;
use std::sync::{LockResult, RwLock, RwLockWriteGuard};

mod sound;
pub use sound::*;

mod source_kind;
pub use source_kind::*;

pub static LOG: &'static str = "audio";

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Sound>();
}

pub struct System {
	_stream: rodio::OutputStream,
	handle: rodio::OutputStreamHandle,
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
				log::info!(target: LOG, "System initialized");
			}
			Err(e) => {
				log::error!(target: LOG, "Failed to initialize system: {}", e);
			}
		});
		let _ = thread.join();
	}

	fn new() -> Result<Self, Error> {
		let (_stream, handle) = rodio::OutputStream::try_default()?;
		Ok(Self { _stream, handle })
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
		let cursor = std::io::Cursor::new(asset.binary);
		let decoder = match asset.kind {
			SourceKind::MP3 => rodio::Decoder::new_mp3(cursor)?,
			SourceKind::WAV => rodio::Decoder::new_wav(cursor)?,
			SourceKind::Vorbis => rodio::Decoder::new_vorbis(cursor)?,
			SourceKind::Flac => rodio::Decoder::new_flac(cursor)?,
		};
		// idk what to actually pass as the rodio sample type
		Ok(self.create_source(decoder))
	}

	fn create_source(&mut self, decoder: rodio::Decoder<std::io::Cursor<Vec<u8>>>) -> Source {
		Source {
			sink: {
				use rodio::Source;
				let sink = rodio::Sink::try_new(&self.handle).unwrap();
				let samples = decoder.convert_samples::<f32>();
				sink.append(samples);
				sink
			},
		}
	}
}

#[derive(Debug)]
pub enum Error {
	RodioError(rodio::StreamError),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::RodioError(ref err) => write!(f, "Rodio Error: {}", err),
		}
	}
}

impl std::error::Error for Error {}

impl From<rodio::StreamError> for Error {
	fn from(err: rodio::StreamError) -> Error {
		Error::RodioError(err)
	}
}

pub struct Source {
	sink: rodio::Sink,
}

impl Source {
	pub fn play(&self) {
		self.sink.play()
	}
}

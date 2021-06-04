use crate::utility::singleton::Singleton;
use rodio;

pub static LOG: &'static str = "audio";

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

	fn get() -> &'static std::sync::RwLock<Self> {
		unsafe { Self::instance() }.get()
	}

	pub fn write() -> std::sync::RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}
}

impl System {

	pub fn create_sound<D>(&mut self, bytes: Vec<u8>) -> Sound
	where
		D: rodio::Sample + Send + Sync + 'static,
	{
		use rodio::Source;
		type Sampler<D> =
			rodio::source::SamplesConverter<rodio::Decoder<std::io::Cursor<Vec<u8>>>, D>;
		let sink = rodio::Sink::try_new(&self.handle).unwrap();
		let source: Sampler<D> = rodio::Decoder::new(std::io::Cursor::new(bytes))
			.unwrap()
			.convert_samples();
		sink.append(source);
		Sound { sink }
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

pub struct Sound {
	sink: rodio::Sink,
}

impl Sound {
	pub fn play(&self) {
		self.sink.play()
	}
}

use crate::{
	network::{self, Socket, SocketEventQueue, LOG},
	utility::{AnyError, VoidResult},
};
use enumset::EnumSet;
use std::sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard};

#[derive(Debug)]
pub struct Config {
	pub mode: EnumSet<network::Kind>,
	pub port: u16,
}

#[derive(Default)]
pub struct Network {
	access: Option<NetAccess>,
	mode: EnumSet<network::Kind>,
}

impl Network {
	pub fn get() -> &'static RwLock<Network> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<Network> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}

	pub fn start(&mut self, config: Config) -> VoidResult {
		assert!(self.access.is_none());
		log::info!(target: LOG, "Starting network with config {:?}", config);
		self.mode = config.mode;
		self.access = Some(NetAccess::new(config)?);
		Ok(())
	}

	pub fn stop(&mut self) {
		self.access = None;
		self.mode.clear();
		log::info!(target: LOG, "Network stopped");
	}

	pub fn process() {
		let queue = match Self::read() {
			Ok(network) => match network.access.as_ref() {
				Some(access) => access.incoming_event_queue.clone(),
				None => return,
			},
			_ => return,
		};
		let mut queue_locked = queue.lock().unwrap();
		while let Some(event) = queue_locked.pop_front() {
			log::debug!("network received {:?}", event);
		}
	}

	pub fn mode(&self) -> &EnumSet<network::Kind> {
		&self.mode
	}
}

struct NetAccess {
	incoming_event_queue: SocketEventQueue,
	socket: Socket,
}

impl NetAccess {
	fn new(config: Config) -> Result<Self, AnyError> {
		let socket = network::Socket::new(config.port)?;
		let incoming_event_queue = socket.create_reception_queue();
		Ok(Self {
			socket,
			incoming_event_queue,
		})
	}
}

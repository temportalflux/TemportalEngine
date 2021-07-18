use crate::{
	network::{
		self,
		event::Event,
		packet::{self, Packet},
		Socket, SocketIncomingQueue, SocketOutgoingQueue, LOG,
	},
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

	pub fn mode(&self) -> &EnumSet<network::Kind> {
		&self.mode
	}

	pub fn process() {
		let queue = match Self::read() {
			Ok(network) => match network.access.as_ref() {
				Some(access) => access.incoming_queue.clone(),
				None => return,
			},
			_ => return,
		};
		let mut queue_locked = queue.lock().unwrap();
		while let Some(event) = queue_locked.pop_front() {
			log::debug!(target: LOG, "received {:?}", event);
			match event {
				Event::Connected(_address) => {}
				Event::TimedOut(_address) => {}
				Event::Disconnected(_address) => {}
				Event::Packet(mut packet) => {
					let (kind_id, packet_data, process_fn) = match packet::Registry::read() {
						Ok(registry) => match packet.take_payload().into_packet(&registry) {
							Some(data) => data,
							None => {
								log::error!(
									target: LOG,
									"Failed to parse packet with kind({})",
									packet.kind()
								);
								return;
							}
						},
						Err(_) => return,
					};
					if let Err(e) = (*process_fn)(packet_data, *packet.address(), *packet.guarantees()) {
						log::error!(
							target: LOG,
							"Failed to process packet with kind({}): {}",
							kind_id,
							e
						);
					}
				}
			}
		}
	}

	/// Enqueues the packet to be sent in the sending thread
	pub fn send(packet: Packet) {
		let queue = match Self::read() {
			Ok(network) => match network.access.as_ref() {
				Some(access) => access.outgoing_queue.clone(),
				None => return,
			},
			_ => return,
		};
		let mut queue_locked = queue.lock().unwrap();
		queue_locked.push_back(packet);
	}
}

struct NetAccess {
	outgoing_queue: SocketOutgoingQueue,
	incoming_queue: SocketIncomingQueue,
	_socket: Socket,
}

impl NetAccess {
	fn new(config: Config) -> Result<Self, AnyError> {
		let socket = network::Socket::new(config.port)?;
		Ok(Self {
			incoming_queue: socket.create_incoming_queue(),
			outgoing_queue: socket.create_outgoing_queue(),
			_socket: socket,
		})
	}
}

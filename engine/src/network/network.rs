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
use std::{
	collections::VecDeque,
	net::SocketAddr,
	sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard, Weak},
};

#[derive(Debug)]
pub struct Config {
	pub mode: EnumSet<network::Kind>,
	pub port: u16,
}

#[derive(Default)]
pub struct Network {
	access: Option<NetAccess>,
	mode: EnumSet<network::Kind>,
	observers: Vec<Weak<RwLock<dyn NetObserver>>>,
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

	pub fn add_observer<T>(&mut self, weak: Weak<RwLock<T>>)
	where
		T: NetObserver + 'static,
	{
		self.observers.push(weak);
	}

	pub fn process() -> VoidResult {
		let queue = match Self::write() {
			Ok(mut network) => {
				network.observers.retain(|o| o.strong_count() > 0);
				match network.access.as_ref() {
					Some(access) => access.incoming_queue.clone(),
					None => return Ok(()),
				}
			}
			_ => return Ok(()),
		};

		let mut events = queue.lock().unwrap().drain(..).collect::<VecDeque<_>>();
		while let Some(event) = events.pop_front() {
			log::debug!(target: LOG, "received {:?}", event);
			if let Event::Packet(mut packet) = event {
				let (kind_id, packet_data, process_fn) = match packet::Registry::read() {
					Ok(registry) => {
						let payload = packet.take_payload();
						match registry.at(payload.kind().as_str()) {
							Some(entry) => (
								payload.kind().clone(),
								entry.deserialize_from(payload.data()),
								entry.process_fn(),
							),
							None => {
								log::error!(
									target: LOG,
									"Failed to parse packet with kind({})",
									packet.kind()
								);
								return Ok(());
							}
						}
					}
					Err(_) => return Ok(()),
				};
				if let Err(e) = (*process_fn)(packet_data, *packet.address(), *packet.guarantees())
				{
					log::error!(
						target: LOG,
						"Failed to process packet with kind({}): {}",
						kind_id,
						e
					);
				}
			} else {
				let observers = match Self::read() {
					Ok(network) => network
						.observers
						.iter()
						.filter_map(|o| o.upgrade())
						.collect::<Vec<_>>(),
					Err(_) => return Ok(()),
				};
				for observer in observers {
					let mut observer_write = observer.write().unwrap();
					match event.clone() {
						Event::Packet(_) => {}
						Event::Connected(address) => observer_write.on_connect(address)?,
						Event::TimedOut(address) => observer_write.on_timeout(address)?,
						Event::Disconnected(address) => observer_write.on_disconnect(address)?,
					}
				}
			}
		}

		Ok(())
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

pub trait NetObserver {
	fn on_connect(&mut self, address: SocketAddr) -> VoidResult {
		Ok(())
	}
	fn on_timeout(&mut self, address: SocketAddr) -> VoidResult {
		Ok(())
	}
	fn on_disconnect(&mut self, address: SocketAddr) -> VoidResult {
		Ok(())
	}
}
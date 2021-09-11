use super::{connection, event, mode, packet, processor, Receiver, Sender};
use crate::utility::{registry::Registerable, VoidResult};
use std::sync::{Arc, Mutex, RwLock};

pub struct Builder {
	connection_list: Arc<RwLock<connection::List>>,
	port: u16,
	processor_registry: Arc<Mutex<processor::Registry>>,
	type_registry: Arc<Mutex<packet::Registry>>,
}

impl Default for Builder {
	fn default() -> Self {
		Self::new().with_default_connection_processors()
	}
}

impl Builder {
	pub fn new() -> Self {
		Self {
			connection_list: Arc::new(RwLock::new(connection::List::default())),
			port: 0,
			processor_registry: Arc::new(Mutex::new(processor::Registry::new())),
			type_registry: Arc::new(Mutex::new(packet::Registry::new())),
		}
	}

	pub fn with_port(mut self, port: u16) -> Self {
		self.set_port(port);
		self
	}

	pub fn set_port(&mut self, port: u16) {
		self.port = port;
	}

	pub fn register_packet<T>(&mut self)
	where
		T: Registerable<packet::KindId, packet::Registration> + 'static,
	{
		if let Ok(mut reg_guard) = self.type_registry.lock() {
			(*reg_guard).register::<T>();
		}
	}

	pub fn with_default_connection_processors(mut self) -> Self {
		self.register_default_connection_processors();
		self
	}

	pub fn register_default_connection_processors(&mut self) {
		if let Ok(mut reg_guard) = self.processor_registry.lock() {
			struct ProcEvent {
				event: event::Kind,
				processors: processor::EventProcessors,
			}
			let mut procs = vec![
				ProcEvent {
					event: event::Kind::Connected,
					processors: processor::EventProcessors::default(),
				},
				ProcEvent {
					event: event::Kind::Disconnected,
					processors: processor::EventProcessors::default(),
				},
			];

			// Modes: Client, Server, Client + Server
			let iter_all_modes = mode::Set::all().iter().chain(mode::Set::all().iter());
			for mode in iter_all_modes {
				for proc in procs.iter_mut() {
					let list = self.connection_list.clone();
					match proc.event {
						event::Kind::Connected => proc
							.processors
							.insert(mode, processor::CreateConnection::new(list)),
						event::Kind::Disconnected => proc
							.processors
							.insert(mode, processor::DestroyConnection::new(list)),
						_ => {}
					}
				}
			}
			for proc in procs.drain(..) {
				(*reg_guard).insert(proc.event, proc.processors);
			}
		}
	}

	pub fn spawn(&self) -> VoidResult {
		let (send_queue, recv_queue) = socknet::start(self.port)?;

		let _sender = Sender {
			connection_list: self.connection_list.clone(),
			receiver_event_sender: recv_queue.sender().clone(),
			queue: send_queue,
		};

		let _receiver = Receiver {
			connection_list: self.connection_list.clone(),
			queue: recv_queue,
			processor_registry: self.processor_registry.clone(),
			type_registry: self.type_registry.clone(),
		};

		Ok(())
	}
}

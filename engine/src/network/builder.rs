use super::{processor, packet, Receiver};
use std::sync::{Arc, RwLock};

pub struct Builder {
	connection_list: Arc<RwLock<connection::List>>,
	port: u16,
	processor_registry: processor::Registry,
	type_registry: packet::Registry,
}

impl Default for Builder {
	fn default() -> Self {
		Self {
			connection_list: Arc::new(RwLock::new(connection::List::default())),
			port: 0,
			processor_registry: processor::Registry::new(),
			type_registry: packet::Registry::new(),
		}
	}
}

impl Builder {

	pub fn with_port(mut self, port: u16) -> Self {
		self.set_port = port;
		self
	}

	pub fn set_port(&mut self, port: u16) {
		self.port = port;
	}

	pub fn with_default_connection_processors(mut self) -> Self {
		self.register_default_connection_processors();
		self
	}

	pub fn register_default_connection_processors(&mut self) {
		// TODO: default behavior for saving connections to the list on connect and disconnect
	}

	pub fn spawn(&self) -> VoidResult {
		let (send_queue, recv_queue) = socknet::start(self.port)?;
	
		let sender = Sender {
			connection_list: self.connection_list.clone(),
			receiver_event_sender: recv_queue.sender().clone(),
			queue: send_queue,
		};

		let receiver = Receiver {
			connection_list: self.connection_list.clone(),
			queue: recv_queue,
			processor_registry: self.processor_registry.clone(),
			type_registry: self.type_registry.clone(),
		};

		Ok(())
	}

}

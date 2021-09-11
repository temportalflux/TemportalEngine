use super::{connection, event, mode, packet, processor, LocalData, Network, Receiver, Sender};
use crate::utility::{registry::Registerable, VoidResult};
use std::sync::{atomic::AtomicBool, Arc, Mutex, RwLock};

pub struct Builder {
	connection_list: Arc<RwLock<connection::List>>,
	local_data: LocalData,
	flag_should_be_destroyed: Arc<AtomicBool>,
	processor_registry: Arc<Mutex<processor::Registry>>,
	type_registry: Arc<Mutex<packet::Registry>>,
}

impl Default for Builder {
	fn default() -> Self {
		Self::new()
			.with_default_connection_processors()
			.with_default_network_stopper()
	}
}

impl Builder {
	pub fn new() -> Self {
		Self {
			connection_list: Arc::new(RwLock::new(connection::List::default())),
			local_data: LocalData::default(),
			flag_should_be_destroyed: Arc::new(AtomicBool::new(false)),
			processor_registry: Arc::new(Mutex::new(processor::Registry::new())),
			type_registry: Arc::new(Mutex::new(packet::Registry::new())),
		}
	}

	pub fn with_mode<TModeSet: Into<mode::Set>>(mut self, modes: TModeSet) -> Self {
		self.insert_modes(modes);
		self
	}

	pub fn insert_modes<TModeSet: Into<mode::Set>>(&mut self, modes: TModeSet) {
		self.local_data.insert_modes(modes);
	}

	pub fn with_port(mut self, port: u16) -> Self {
		self.set_port(port);
		self
	}

	pub fn set_port(&mut self, port: u16) {
		self.local_data.set_port(port);
	}

	pub fn with_registrations_in<F>(mut self, callback: F) -> Self
	where
		F: Fn(&mut Self),
	{
		callback(&mut self);
		self
	}

	pub fn register_packet<T>(&mut self)
	where
		T: Registerable<packet::KindId, packet::Registration> + 'static,
	{
		if let Ok(mut reg_guard) = self.type_registry.lock() {
			(*reg_guard).register::<T>();
		}
	}

	pub fn register_processors(
		&mut self,
		event: event::Kind,
		processors: processor::EventProcessors,
	) {
		if let Ok(mut reg_guard) = self.processor_registry.lock() {
			(*reg_guard).insert(event, processors);
		}
	}

	pub fn add_processor<TNetMode, TProc>(
		&mut self,
		event: event::Kind,
		net_modes: impl std::iter::Iterator<Item = TNetMode>,
		processor: TProc,
	) where
		TNetMode: Into<mode::Set>,
		TProc: processor::Processor + Clone + 'static,
	{
		let mut reg_guard = self.processor_registry.lock().unwrap();
		if !(*reg_guard).contains(&event) {
			(*reg_guard).insert(event.clone(), processor::EventProcessors::default());
		}
		let event_procs = (*reg_guard).get_mut(&event).unwrap();
		for mode in net_modes {
			event_procs.insert(mode.into(), processor.clone());
		}
	}

	pub fn register_bundle<T>(&mut self, processors: processor::EventProcessors)
	where
		T: Registerable<packet::KindId, packet::Registration> + 'static,
	{
		self.register_packet::<T>();
		self.register_processors(event::Kind::Packet(T::unique_id().to_owned()), processors);
	}

	pub fn with_default_connection_processors(mut self) -> Self {
		self.register_default_connection_processors();
		self
	}

	pub fn register_default_connection_processors(&mut self) {
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

		for mode in mode::all() {
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

		let mut reg_guard = self.processor_registry.lock().unwrap();
		for proc in procs.drain(..) {
			(*reg_guard).insert(proc.event, proc.processors);
		}
	}

	pub fn with_default_network_stopper(mut self) -> Self {
		self.register_default_network_stopper();
		self
	}

	pub fn register_default_network_stopper(&mut self) {
		let mut processor = processor::EventProcessors::default();
		for mode in mode::all() {
			processor.insert(
				mode,
				processor::EndNetwork::new(self.flag_should_be_destroyed.clone()),
			);
		}

		let mut reg_guard = self.processor_registry.lock().unwrap();
		(*reg_guard).insert(event::Kind::Stop, processor);
	}

	pub fn spawn(&self) -> VoidResult {
		if Network::is_active() {
			return Err(Box::new(super::Error::NetworkAlreadyActive()));
		}

		let (send_queue, recv_queue) =
			socknet::start(self.local_data.port(), &self.flag_should_be_destroyed)?;

		let sender = Sender {
			local_data: self.local_data.clone(),
			queue: send_queue,
			receiver_event_sender: recv_queue.sender().clone(),
			connection_list: self.connection_list.clone(),
		};

		let receiver = Receiver {
			local_data: self.local_data.clone(),
			processor_registry: self.processor_registry.clone(),
			type_registry: self.type_registry.clone(),
			queue: recv_queue,
			flag_should_be_destroyed: self.flag_should_be_destroyed.clone(),
			connection_list: self.connection_list.clone(),
		};

		Network::receiver_init(receiver)?;
		Network::sender_init(sender)?;

		Ok(())
	}
}

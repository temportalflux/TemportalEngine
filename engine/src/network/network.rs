use super::{packet, LocalData, Receiver, Sender, LOG};
use crate::utility::Result;
use std::{
	mem::MaybeUninit,
	sync::{atomic, Mutex, Once},
};

#[derive(Default)]
pub struct Network {}

impl Network {
	pub fn receiver() -> &'static Mutex<Option<Receiver>> {
		static mut INSTANCE: (MaybeUninit<Mutex<Option<Receiver>>>, Once) =
			(MaybeUninit::uninit(), Once::new());
		unsafe {
			INSTANCE
				.1
				.call_once(|| INSTANCE.0.as_mut_ptr().write(Mutex::new(None)));
			&*INSTANCE.0.as_ptr()
		}
	}

	pub(super) fn receiver_init(receiver: Receiver) {
		let mut guard = Network::receiver().lock().unwrap();
		(*guard) = Some(receiver);
	}

	fn sender() -> &'static Mutex<Option<Sender>> {
		static mut INSTANCE: (MaybeUninit<Mutex<Option<Sender>>>, Once) =
			(MaybeUninit::uninit(), Once::new());
		unsafe {
			INSTANCE
				.1
				.call_once(|| INSTANCE.0.as_mut_ptr().write(Mutex::new(None)));
			&*INSTANCE.0.as_ptr()
		}
	}

	pub(super) fn sender_init(sender: Sender) {
		let mut guard = Network::sender().lock().unwrap();
		(*guard) = Some(sender);
	}

	pub fn is_active() -> bool {
		// NOTE: We only check the sender here because the sender will
		// only be locked when a packet is being enqueued (and therefore
		// has the lowest possible chance of ever causing a deadlock).
		// If we were checking the receiver, deadlock would be more prone
		// to happen (i.e. calling `is_active` in the callstack of packet processing).
		if let Ok(guard) = Network::sender().lock() {
			return (*guard).is_some();
		}
		false
	}

	pub fn local_data() -> LocalData {
		if let Ok(guard) = Network::sender().lock() {
			if let Some(sender) = &*guard {
				return sender.local_data.clone();
			}
		}
		LocalData::default()
	}

	pub fn destroy() -> Result<()> {
		log::info!(target: LOG, "Destroying network");
		if let Ok(mut guard) = Network::receiver().lock() {
			// If destroy is called by natural process completion (instead of the Stop event),
			// then the exit flag may not have been signaled, leaving network threads hanging.
			// So lets just ensure that the exit flag is always marked before dropping stuff.
			if let Some(receiver) = &*guard {
				receiver
					.flag_should_be_destroyed
					.store(true, atomic::Ordering::Relaxed);
			}
			(*guard) = None;
		}
		if let Ok(mut guard) = Network::sender().lock() {
			(*guard) = None;
		}
		Ok(())
	}

	pub fn stop() -> Result<()> {
		if let Ok(guard) = Network::sender().lock() {
			if let Some(sender) = &*guard {
				sender.stop()?;
			}
		}
		Ok(())
	}

	/// Enqueues the packet to be sent in the sending thread
	#[profiling::function]
	pub fn send_packets(builder: packet::PacketBuilder) -> Result<()> {
		if let Ok(guard) = Network::sender().lock() {
			if let Some(sender) = &*guard {
				sender.send_packets(builder)?;
			}
		}
		Ok(())
	}

	#[profiling::function]
	pub fn send_all_packets(builders: Vec<packet::PacketBuilder>) -> Result<()> {
		if let Ok(guard) = Network::sender().lock() {
			if let Some(sender) = &*guard {
				for builder in builders.into_iter() {
					sender.send_packets(builder)?;
				}
			}
		}
		Ok(())
	}

	pub fn kick(address: &std::net::SocketAddr) -> Result<()> {
		if let Ok(guard) = Network::sender().lock() {
			if let Some(sender) = &*guard {
				sender.kick(&address)?;
			}
		}
		Ok(())
	}
}

pub type AnyError = Box<dyn std::error::Error>;
pub type VoidResult = std::result::Result<(), AnyError>;

pub static LOG: &'static str = "socknet";

#[cfg(feature = "derive")]
pub use socknet_derive::packet_kind;

pub mod backend {
	pub use laminar::{Config, Connection, ConnectionManager, Packet, SocketEvent as Event};
}
pub use crossbeam_channel as channel;
pub mod event;
pub mod packet;
pub mod socket;

pub mod serde {
	pub use rmp_serde::{from_read_ref, to_vec};
}

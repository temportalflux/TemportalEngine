pub type AnyError = Box<dyn std::error::Error>;
pub type VoidResult = std::result::Result<(), AnyError>;

#[cfg(feature = "derive")]
pub use socknet_derive::packet_kind;

pub mod event;
pub mod packet;

mod socket;
pub use socket::*;

pub mod serde {
	pub use rmp_serde::{from_read_ref, to_vec};
}

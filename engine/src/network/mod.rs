pub static LOG: &'static str = "network";

pub use socknet::*;

pub mod connection;
pub mod packet;

mod kind;
pub use kind::*;

mod network;
pub use network::*;

pub mod prelude {
	pub use super::{
		connection::Connection,
		network::Network,
		packet::{DeliveryGuarantee, Guarantee, OrderGuarantee, Packet},
		Kind,
	};
}

pub fn mode() -> enumset::EnumSet<Kind> {
	Network::read().unwrap().mode().clone()
}

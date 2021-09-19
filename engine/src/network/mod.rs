pub static LOG: &'static str = "network";

pub use socknet::*;

mod builder;
pub use builder::*;

pub mod connection;

pub mod event;

mod error;
pub use error::*;

mod local_data;
pub use local_data::*;

pub mod mode;

mod network;
pub use network::*;

pub mod packet {
	pub use socknet::packet::{
		AnyBox, DeliveryGuarantee, Guarantee, Kind, KindId, OrderGuarantee, Packet, PacketBuilder,
		PacketMode, Payload, Queue, Registerable, Registration, Registry,
	};
}

pub mod prelude {
	pub use super::{
		connection::Connection,
		mode,
		network::Network,
		packet::{DeliveryGuarantee, Guarantee, OrderGuarantee, Packet, PacketMode},
	};
	pub use mode::Kind::*;
	pub use DeliveryGuarantee::*;
	pub use OrderGuarantee::*;
	pub use PacketMode::*;
}

pub mod processor;

mod receiver;
pub use receiver::*;

mod sender;
pub use sender::*;

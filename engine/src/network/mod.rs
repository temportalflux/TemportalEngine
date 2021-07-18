pub static LOG: &'static str = "network";

pub mod connection;
pub mod event;
pub mod packet;

mod kind;
pub use kind::*;

mod network;
pub use network::*;

mod socket;
pub use socket::*;

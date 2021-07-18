pub static LOG: &'static str = "network";

pub use socknet::*;

pub mod connection;
pub mod packet;

mod kind;
pub use kind::*;

mod network;
pub use network::*;

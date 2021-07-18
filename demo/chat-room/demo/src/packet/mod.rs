mod handshake;
pub use handshake::*;
mod message;
pub use message::*;

pub fn register_types() {
	use crate::engine::network::packet::Registry;
	Registry::register::<Handshake>();
	Registry::register::<Message>();
}

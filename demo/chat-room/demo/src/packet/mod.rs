mod handshake;
pub use handshake::*;

pub fn register_types() {
	use crate::engine::network::packet::Registry;
	Registry::register::<Handshake>();
}

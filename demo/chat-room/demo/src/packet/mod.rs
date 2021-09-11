mod handshake;
pub use handshake::*;
mod message;
pub use message::*;

mod connection;
pub use connection::*;

use crate::engine::network;
pub fn register_types(builder: &mut network::Builder) {
	Handshake::register(builder);
	Message::register(builder);
	connection::register_bonus_processors(builder);
}

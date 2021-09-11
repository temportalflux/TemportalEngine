#[derive(Debug)]
pub enum Error {
	NetworkAlreadyActive(),
	EncounteredNonPacket(super::event::Kind),
}

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match self {
			Error::NetworkAlreadyActive() => {
				write!(f, "Cannot spawn a network when it is already active")
			}
			Error::EncounteredNonPacket(ref kind) => write!(
				f,
				"Packet-Only processor cannot handle non-packet event: {}",
				kind
			),
		}
	}
}

impl std::error::Error for Error {}

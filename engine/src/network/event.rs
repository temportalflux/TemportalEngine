
#[derive(Debug, Clone)]
pub enum Kind {
	Connected,
	Packet(/*packet id*/ String),
	Timeout,
	Disconnected,
	Stop,
}

impl std::fmt::Display for Kind {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "{:?}", self)
	}
}

pub enum Data {
	Address(std::net::SocketAddr),
	Packet(std::net::SocketAddr, super::packet::Guarantee, super::packet::AnyBox),
}

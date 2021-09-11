#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Kind {
	Connected,
	Packet(/*packet id*/ String),
	Timeout,
	Disconnected,
	Stop,
}

impl std::fmt::Display for Kind {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Self::Connected => write!(f, "Connected"),
			Self::Timeout => write!(f, "Timedout"),
			Self::Disconnected => write!(f, "Disconnected"),
			Self::Stop => write!(f, "Stop"),
			Self::Packet(ref packet_kind) => write!(f, "Packet({})", packet_kind),
		}
	}
}

pub enum Data {
	Connection(super::connection::Connection),
	Packet(
		super::connection::Connection,
		super::packet::Guarantee,
		super::packet::AnyBox,
	),
}

impl std::fmt::Display for Data {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Self::Connection(ref connection) => write!(f, "{}", connection),
			Self::Packet(ref connection, ref guarantee, _) => {
				write!(f, "Data({} with {})", connection, guarantee)
			}
		}
	}
}

use super::{connection::Connection, mode};
use std::net::{IpAddr, Ipv4Addr, SocketAddr};

#[derive(Debug, Clone)]
pub struct LocalData {
	pub mode: mode::Set,
	pub address: SocketAddr,
}

impl Default for LocalData {
	fn default() -> Self {
		Self {
			mode: mode::Set::empty(),
			address: SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 0),
		}
	}
}

impl LocalData {
	pub fn insert_modes<TModeSet: Into<mode::Set>>(&mut self, modes: TModeSet) {
		self.mode.insert_all(modes.into());
	}

	pub fn set_port(&mut self, port: u16) {
		self.address.set_port(port);
	}

	pub fn port(&self) -> u16 {
		self.address.port()
	}

	pub fn is_local(&self, connection: &Connection) -> bool {
		self.address == connection.address
	}

	pub fn is_server(&self) -> bool {
		self.mode.contains(mode::Kind::Server)
	}

	pub fn is_client(&self) -> bool {
		self.mode.contains(mode::Kind::Client)
	}

	pub fn is_dedicated(&self, mode: mode::Kind) -> bool {
		self.mode == mode
	}
}

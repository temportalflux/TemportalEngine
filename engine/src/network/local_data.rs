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

impl std::fmt::Display for LocalData {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"NetData(Modes:{}, Port:{})",
			self.mode
				.iter()
				.map(|kind| kind.to_string())
				.collect::<Vec<_>>()
				.join("+"),
			self.port()
		)
	}
}

impl LocalData {
	pub fn with_args(mut self) -> Self {
		self.read_from_args();
		self
	}

	pub fn read_from_args(&mut self) {
		if std::env::args().any(|arg| arg == "-server") {
			self.insert_modes(mode::Kind::Server);
		}
		if std::env::args().any(|arg| arg == "-client") {
			self.insert_modes(mode::Kind::Client);
		}
		self.set_port(Self::get_named_arg("port").unwrap_or(self.port()));
	}

	pub fn get_named_arg(name: &str) -> Option<u16> {
		std::env::args().find_map(|arg| {
			let prefix = format!("-{}=", name);
			arg.strip_prefix(&prefix)
				.map(|s| s.parse::<u16>().ok())
				.flatten()
		})
	}

	pub fn insert_modes<TModeSet: Into<mode::Set>>(&mut self, modes: TModeSet) {
		self.mode.insert_all(modes.into());
	}

	pub fn set_port(&mut self, port: u16) {
		self.address.set_port(port);
	}

	pub fn port(&self) -> u16 {
		self.address.port()
	}

	pub fn address(&self) -> &SocketAddr {
		&self.address
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

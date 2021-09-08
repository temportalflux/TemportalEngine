use std::{
	collections::{HashMap, HashSet},
	net::SocketAddr,
};

#[derive(Default, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Id(usize);

impl std::fmt::Display for Id {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Connection({})", self.0)
	}
}

impl Id {
	pub fn as_address(&self) -> Option<SocketAddr> {
		use super::Network;
		return match Network::read() {
			Ok(ref network) => network.connection_list.get_address(self).map(|v| *v),
			Err(_) => None,
		};
	}
}

pub struct Connection {
	pub id: Option<Id>,
	pub address: SocketAddr,
}

pub struct List {
	pub(super) active_connections: HashMap<Id, SocketAddr>,
	address_connections: HashMap<SocketAddr, Id>,
	unused_ids: HashSet<usize>,
	pub(super) connection_count: usize,
}

impl Default for List {
	fn default() -> Self {
		Self {
			active_connections: HashMap::new(),
			address_connections: HashMap::new(),
			unused_ids: HashSet::new(),
			connection_count: 0,
		}
	}
}

impl List {
	pub fn add_connection(&mut self, address: &SocketAddr) -> Id {
		let conn_id = self.next_connection_id();
		self.active_connections
			.insert(conn_id.clone(), address.clone());
		self.address_connections
			.insert(address.clone(), conn_id.clone());
		self.connection_count += 1;
		return conn_id;
	}

	pub fn remove_connection(&mut self, address: &SocketAddr) -> Option<Id> {
		if let Some(conn_id) = self.address_connections.remove(address) {
			self.active_connections.remove(&conn_id);
			self.unused_ids.insert(conn_id.0);
			self.connection_count -= 1;
			return Some(conn_id);
		}
		return None;
	}

	fn next_connection_id(&mut self) -> Id {
		return match self.unused_ids.iter().next().map(|id| *id) {
			Some(id) => {
				self.unused_ids.remove(&id);
				Id(id)
			}
			None => Id(self.connection_count),
		};
	}

	pub fn get_id(&self, address: &SocketAddr) -> Option<&Id> {
		self.address_connections.get(address)
	}

	pub fn get_address(&self, connection_id: &Id) -> Option<&SocketAddr> {
		self.active_connections.get(connection_id)
	}

	pub fn get_connection(&self, connection_id: &Id) -> Option<Connection> {
		match self.get_address(connection_id) {
			Some(addr) => Some(Connection {
				id: Some(*connection_id),
				address: *addr,
			}),
			None => None,
		}
	}
}

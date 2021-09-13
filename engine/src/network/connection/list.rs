use super::{Connection, Id};
use std::{
	collections::{HashMap, HashSet},
	net::SocketAddr,
};

pub struct List {
	active_connections: HashMap<Id, SocketAddr>,
	address_connections: HashMap<SocketAddr, Id>,
	unused_ids: HashSet<usize>,
}

impl Default for List {
	fn default() -> Self {
		Self {
			active_connections: HashMap::new(),
			address_connections: HashMap::new(),
			unused_ids: HashSet::new(),
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
		return conn_id;
	}

	pub fn remove_connection(&mut self, address: &SocketAddr) -> Option<Id> {
		if let Some(conn_id) = self.address_connections.remove(address) {
			self.active_connections.remove(&conn_id);
			self.unused_ids.insert(conn_id.0);
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
			None => Id(self.connection_count()),
		};
	}

	pub fn iter(&self) -> impl std::iter::Iterator<Item = Connection> + '_ {
		self.active_connections.iter().map(|(id, addr)| Connection {
			id: Some(*id),
			address: *addr,
		})
	}

	pub fn connection_count(&self) -> usize {
		self.active_connections.len()
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

	pub fn get_connection_by_addr(&self, address: &SocketAddr) -> Option<Connection> {
		match self.get_id(address) {
			Some(connection_id) => Some(Connection {
				id: Some(*connection_id),
				address: *address,
			}),
			None => None,
		}
	}
}

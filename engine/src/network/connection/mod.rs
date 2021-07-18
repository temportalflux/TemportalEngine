use std::{
	collections::{HashMap, HashSet},
	net::SocketAddr,
};

pub struct ConnectionId(usize);

pub struct List {
	active_connections: HashMap<ConnectionId, SocketAddr>,
	address_connections: HashMap<SocketAddr, ConnectionId>,
	unused_ids: HashSet<ConnectionId>,
}

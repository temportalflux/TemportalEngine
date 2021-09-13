use super::Id;
use std::net::SocketAddr;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Connection {
	pub id: Option<Id>,
	pub address: SocketAddr,
}

impl std::fmt::Display for Connection {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"Connection(Id:{}, Address:{})",
			match self.id {
				Some(id) => id.to_string(),
				None => "None".to_owned(),
			},
			self.address
		)
	}
}

use super::{
	super::{event, processor::Processor, LocalData, LOG},
	Connection, List,
};
use crate::utility::VoidResult;
use std::sync::{Arc, RwLock};

pub struct DestroyConnection {
	connection_list: Arc<RwLock<List>>,
}

impl DestroyConnection {
	pub fn new(connection_list: Arc<RwLock<List>>) -> Self {
		Self { connection_list }
	}
}

impl Processor for DestroyConnection {
	fn process(
		&self,
		_kind: &event::Kind,
		data: &mut Option<event::Data>,
		_local_data: &LocalData,
	) -> VoidResult {
		if let Some(event::Data::Connection(Connection { address, .. })) = data {
			if let Ok(mut list) = self.connection_list.write() {
				if let Some(conn_id) = list.remove_connection(&address) {
					log::info!(target: LOG, "{} has disconnected as {}", address, conn_id);
				}
			}
		}
		Ok(())
	}
}

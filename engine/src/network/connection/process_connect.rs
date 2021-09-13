use super::{
	super::{event, processor::Processor, LocalData, LOG},
	Connection, List,
};
use crate::utility::VoidResult;
use std::sync::{Arc, RwLock};

pub struct CreateConnection {
	connection_list: Arc<RwLock<List>>,
}

impl CreateConnection {
	pub fn new(connection_list: Arc<RwLock<List>>) -> Self {
		Self { connection_list }
	}
}

impl Processor for CreateConnection {
	fn process(
		&self,
		_kind: &event::Kind,
		data: &mut Option<event::Data>,
		_local_data: &LocalData,
	) -> VoidResult {
		if let Some(event::Data::Connection(Connection { address, id })) = data {
			if let Ok(mut list) = self.connection_list.write() {
				*id = Some(list.add_connection(&address));
				log::info!(target: LOG, "{} has connected as {}", address, id.unwrap());
			}
		}
		Ok(())
	}
}

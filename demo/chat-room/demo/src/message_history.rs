use crate::engine::network::connection;
use std::{
	collections::HashMap,
	net::SocketAddr,
	sync::{LockResult, RwLock, RwLockReadGuard, RwLockWriteGuard},
};

pub struct Message {
	pub sender_name: Option<String>,
	pub content: String,
}

impl Message {
	pub fn display_name(&self) -> String {
		match &self.sender_name {
			Some(name) => name.clone(),
			None => "Server".to_owned(),
		}
	}

	pub fn to_display_text(&self) -> String {
		format!("{}: {}", self.display_name(), self.content)
	}
}

#[derive(Default)]
pub struct MessageHistory {
	user_names: HashMap<connection::Id, String>,
	pending_users: HashMap<SocketAddr, String>,
	messages: Vec<Message>,
}

impl MessageHistory {
	fn get() -> &'static RwLock<Self> {
		use temportal_engine::utility::singleton::*;
		static mut INSTANCE: Singleton<MessageHistory> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}
}

impl MessageHistory {
	pub fn iter_messages(&self) -> std::slice::Iter<Message> {
		self.messages.iter()
	}

	pub fn add(&mut self, message: Message) {
		self.messages.push(message);
	}

	pub fn add_pending_user(&mut self, address: SocketAddr, name: String) {
		self.pending_users.insert(address, name);
	}

	pub fn confirm_user(&mut self, connection: &connection::Connection) {
		if let Some(name) = self.pending_users.remove(&connection.address) {
			self.user_names.insert(connection.id.unwrap(), name);
		}
	}

	pub fn remove_user(&mut self, connection: &connection::Connection) {
		self.user_names.remove(&connection.id.unwrap());
	}

	pub fn get_user(&self, id: &connection::Id) -> Option<&String> {
		self.user_names.get(id)
	}
}

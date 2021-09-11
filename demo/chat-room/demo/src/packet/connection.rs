use crate::engine::{
	network::{
		self, event, mode,
		packet::{DeliveryGuarantee::*, OrderGuarantee::*, Packet},
		processor::Processor,
		LocalData, Network,
	},
	utility::VoidResult,
};

pub fn register_bonus_processors(builder: &mut network::Builder) {
	use event::Kind::*;
	builder.add_processor(Connected, mode::all().into_iter(), ConfirmUser());
}

#[derive(Clone)]
pub struct ConfirmUser();

impl ConfirmUser {
	pub fn announce_arrival(local_data: &LocalData, history: &mut crate::MessageHistory, name: &String) -> VoidResult {
		let message = crate::packet::Message {
			timestamp: chrono::Utc::now(),
			sender_name: None,
			content: format!("{} has joined the server", name),
		};
		Network::broadcast(
			Packet::builder()
				.with_guarantee(Reliable + Ordered)
				.with_payload(&message),
		)?;
		if local_data.is_client() {
			super::SaveMessageToLog::save_to_log(history, &message);
		}
		Ok(())
	}
}

impl Processor for ConfirmUser {
	fn process(
		&self,
		_kind: &event::Kind,
		data: &mut Option<event::Data>,
		local_data: &LocalData,
	) -> VoidResult {
		if let Some(event::Data::Connection(connection)) = data {
			if let Ok(mut history) = crate::MessageHistory::write() {
				history.confirm_user(&connection);
				if let Some(user_name) = history.get_user(&connection.id.unwrap()).cloned() {
					Self::announce_arrival(local_data, &mut history, &user_name)?;
				}
			}
		}
		Ok(())
	}
}

#[derive(Clone)]
struct DestroyUser();

impl Processor for DestroyUser {
	fn process(
		&self,
		_kind: &event::Kind,
		data: &mut Option<event::Data>,
		_local_data: &LocalData,
	) -> VoidResult {
		if let Some(event::Data::Connection(connection)) = data {
			if let Ok(mut history) = crate::MessageHistory::write() {
				Network::broadcast(
					Packet::builder()
						.with_guarantee(Reliable + Ordered)
						.with_payload(&crate::packet::Message {
							timestamp: chrono::Utc::now(),
							sender_name: None,
							content: format!(
								"{} has left the server",
								history.get_user(&connection.id.unwrap()).unwrap()
							),
						}),
				)?;

				history.remove_user(&connection);
			}
		}
		Ok(())
	}
}

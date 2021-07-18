use super::Kind;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct Payload {
	kind_id: String,
	data: Vec<u8>,
}

impl std::fmt::Debug for Payload {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		write!(
			f,
			"packet-kind:{} ({} bytes)",
			self.kind_id,
			self.data.len()
		)
	}
}

impl<TKind> From<&TKind> for Payload
where
	TKind: Kind,
{
	fn from(kind: &TKind) -> Self {
		Self {
			kind_id: TKind::unique_id().into(),
			data: kind.serialize_to(),
		}
	}
}

impl Payload {
	pub fn kind(&self) -> &String {
		&self.kind_id
	}

	pub fn data(&self) -> &[u8] {
		&self.data[..]
	}

	pub fn take(&mut self) -> Payload {
		Payload {
			kind_id: self.kind_id.clone(),
			data: self.data.drain(..).collect(),
		}
	}
}

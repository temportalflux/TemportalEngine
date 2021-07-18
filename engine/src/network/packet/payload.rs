use super::{Kind, KindIdOwned, Registry, FnProcessKind, AnyBox};
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct Payload {
	kind_id: KindIdOwned,
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
	pub fn kind(&self) -> &KindIdOwned {
		&self.kind_id
	}

	pub fn take(&mut self) -> Payload {
		Payload {
			kind_id: self.kind_id.clone(),
			data: self.data.drain(..).collect(),
		}
	}

	pub fn into_packet(self, registry: &Registry) -> Option<(KindIdOwned, AnyBox, FnProcessKind)> {
		registry
			.at(self.kind_id.as_str())
			.map(|entry| (self.kind_id, entry.deserialize_from(&self.data[..]), entry.process_fn()))
	}
}

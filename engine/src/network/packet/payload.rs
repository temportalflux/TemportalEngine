use super::{Kind, KindIdOwned, Registry};
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct Payload {
	kind_id: KindIdOwned,
	data: Vec<u8>,
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
	pub fn into_packet(self, registry: &Registry) -> Option<(KindIdOwned, Box<dyn Kind>)> {
		registry
			.at(self.kind_id.as_str())
			.map(|entry| (self.kind_id, entry.deserialize_from(&self.data[..])))
	}
}

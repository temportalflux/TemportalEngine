use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::flags::{AttachmentKind, ImageLayout},
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Subpass).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Subpass {
	asset_type: String,
	pub(crate) attachments: Vec<AttachmentItem>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub(crate) struct AttachmentItem {
	pub id: asset::Id,
	pub kind: AttachmentKind,
	pub layout: Option<ImageLayout>,
}

impl asset::Asset for Subpass {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(SubpassMetadata {})
	}
}

/// The metadata about the [`Subpass`] asset type.
pub struct SubpassMetadata;
impl TypeMetadata for SubpassMetadata {
	fn name(&self) -> asset::TypeId {
		"render-subpass"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		asset::decompile_asset::<Subpass>(bin)
	}
}

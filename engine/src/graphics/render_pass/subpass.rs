use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::flags::ImageLayout,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Subpass).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Subpass {
	asset_type: String,
	pub(crate) attachments: AttachmentSet,
}

#[derive(Serialize, Deserialize, Default, Debug, Clone)]
pub(crate) struct AttachmentSet {
	#[serde(default)]
	pub input: Vec<AttachmentItem>,
	#[serde(default)]
	pub color: Vec<AttachmentItem>,
	#[serde(default)]
	pub depth_stencil: Option<AttachmentItem>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub(crate) struct AttachmentItem {
	pub id: asset::Id,
	pub layout: ImageLayout,
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

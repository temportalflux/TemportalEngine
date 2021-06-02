use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::flags::ImageLayout,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Subpass).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Subpass {
	asset_type: String,
	attachments: AttachmentSet,
}

#[derive(Serialize, Deserialize, Default, Debug, Clone)]
struct AttachmentSet {
	#[serde(default)]
	input: Vec<AttachmentItem>,
	#[serde(default)]
	color: Vec<AttachmentItem>,
	#[serde(default)]
	depth: Vec<AttachmentItem>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct AttachmentItem {
	id: asset::Id,
	layout: ImageLayout,
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

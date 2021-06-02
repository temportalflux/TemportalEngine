use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::{
		flags::{format, ImageLayout, SampleCount},
		renderpass::AttachmentOps,
	},
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Attachment).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Attachment {
	asset_type: String,

	format: format::Components,

	#[serde(default)]
	sample_count: SampleCount,

	#[serde(default)]
	operations: Ops,

	#[serde(default)]
	initial_layout: ImageLayout,

	layout: ImageLayout,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
struct Ops {
	#[serde(default)]
	general: AttachmentOps,
	#[serde(default)]
	stencil: AttachmentOps,
}

impl asset::Asset for Attachment {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(AttachmentMetadata {})
	}
}

/// The metadata about the [`Attachment`] asset type.
pub struct AttachmentMetadata;
impl TypeMetadata for AttachmentMetadata {
	fn name(&self) -> asset::TypeId {
		"render-attachment"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		asset::decompile_asset::<Attachment>(bin)
	}
}

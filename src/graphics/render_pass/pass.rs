use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::flags,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Pass).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Pass {
	asset_type: String,
	/// Defines the order that subpasses will be recorded in the [`RenderChain`](crate::graphics::RenderChain).
	/// This is not the order that the GPU will process the commands - that is defined by [`dependencies`](Pass::dependencies).
	subpass_order: Vec<asset::Id>,
	/// Defines what subpasses are required by other subpasses (and which is first) so the GPU
	/// knows how to composite the subpasses together.
	dependencies: Vec<Dependency>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct Dependency {
	first: DependencyItem,
	then: DependencyItem,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct DependencyItem {
	subpass: Option<asset::Id>,
	#[serde(default)]
	stage: Vec<flags::PipelineStage>,
	#[serde(default)]
	access: Vec<flags::Access>,
}

impl asset::Asset for Pass {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(PassMetadata {})
	}
}

/// The metadata about the [`Pass`] asset type.
pub struct PassMetadata;
impl TypeMetadata for PassMetadata {
	fn name(&self) -> asset::TypeId {
		"render-pass"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		asset::decompile_asset::<Pass>(bin)
	}
}

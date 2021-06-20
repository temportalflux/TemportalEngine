use crate::{
	asset::{self, AssetResult, TypeMetadata},
	audio::SourceKind,
};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Sound {
	asset_type: String,

	kind: SourceKind,

	#[serde(default)]
	binary: Vec<u8>,
}

impl Sound {
	pub fn kind(&self) -> SourceKind {
		self.kind
	}

	pub fn set_binary(&mut self, binary: Vec<u8>) {
		self.binary = binary;
	}

	pub fn binary(&self) -> &Vec<u8> {
		&self.binary
	}
}

impl asset::Asset for Sound {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(Metadata {})
	}
}

struct Metadata;

impl TypeMetadata for Metadata {
	fn name(&self) -> asset::TypeId {
		"sound"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		asset::decompile_asset::<Sound>(bin)
	}
}

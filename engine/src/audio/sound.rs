use crate::{
	asset::{self},
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
	fn asset_type() -> asset::TypeId {
		"sound"
	}

	fn decompile(bin: &Vec<u8>) -> anyhow::Result<asset::AnyBox> {
		asset::decompile_asset::<Self>(bin)
	}
}

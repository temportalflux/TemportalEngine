use crate::asset::{self, AssetResult, TypeMetadata};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Font {
	asset_type: String,
}

impl asset::Asset for Font {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(FontMetadata {})
	}
}

impl Font {}

pub struct FontMetadata {}

impl TypeMetadata for FontMetadata {
	fn name(&self) -> asset::TypeId {
		"font"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		let shader: Font = rmp_serde::from_read_ref(&bin)?;
		Ok(Box::new(shader))
	}
}

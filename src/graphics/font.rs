use crate::{asset::{self, AssetResult, TypeMetadata}, math::Vector};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Font {
	asset_type: String,
}

//#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Glyph {
	pub ascii_code: usize,
	// The position of the glyph in the atlas texture.
	pub atlas_pos: Vector<usize, 2>,
	// The size of the glyph in the atlas texture.
	pub atlas_size: Vector<usize, 2>,
	// The size of the glyph in metrics.
	pub metric_size: Vector<usize, 2>,
	// The bearing meteric, where x is the distance to the right of the pen position,
	// and y is the distance up from the pen position,
	// that the glyph's top-left position should be rendered at.
	pub metric_bearing: Vector<usize, 2>,
	// The amount of space to the right that the pen position should move,
	// when the glyph is rendered.
	pub metric_advance: usize,
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

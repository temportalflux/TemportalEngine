use crate::{
	asset::{self, AssetResult, TypeMetadata},
	math::Vector,
};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Font {
	asset_type: String,
	sdf: Option<SDF>,
}

#[derive(Serialize, Deserialize, Clone)]
pub struct SDF {
	pub size: Vector<usize, 2>,
	pub binary: Vec<Vec<u8>>,
	pub glyphs: Vec<Glyph>,
}

impl std::fmt::Debug for SDF {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(
			f,
			"SDL {{ size:{:?}, glyphs:{:?}, binary:(omitted) }}",
			self.size,
			self.glyphs.len()
		)
	}
}

#[derive(Serialize, Deserialize, Clone)]
pub struct Glyph {
	pub unicode: u32,
	// The position (in pixels) of the glyph in the atlas texture.
	pub atlas_pos: Vector<usize, 2>,
	// The size (in pixels) of the glyph in the atlas texture.
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

impl Glyph {
	pub fn as_char(&self) -> char {
		char::from_u32(self.unicode as u32).unwrap()
	}
}

impl asset::Asset for Font {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(FontMetadata {})
	}
}

impl Font {
	pub fn set_sdf(&mut self, sdf: SDF) {
		self.sdf = Some(sdf);
	}

	pub fn glyphs(&self) -> &Vec<Glyph> {
		&self.sdf.as_ref().unwrap().glyphs
	}

	pub fn binary(&self) -> &Vec<Vec<u8>> {
		&self.sdf.as_ref().unwrap().binary
	}

	pub fn size(&self) -> &Vector<usize, 2> {
		&self.sdf.as_ref().unwrap().size
	}
}

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

use crate::{
	asset::{self, AssetResult, TypeMetadata},
	math::Vector,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of [`images`](crate::graphics::image::Image).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Texture {
	asset_type: String,
	compiled: Option<CompiledTexture>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub(crate) struct CompiledTexture {
	pub(crate) size: Vector<usize, 2>,
	pub(crate) binary: Vec<u8>,
}

impl asset::Asset for Texture {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(TextureMetadata {})
	}
}

impl Texture {
	#[doc(hidden)]
	pub fn set_compiled(&mut self, size: Vector<usize, 2>, binary: Vec<u8>) {
		self.compiled = Some(CompiledTexture { size, binary })
	}

	/// Returns the <width, height> of the image/texture.
	pub fn size(&self) -> &Vector<usize, 2> {
		&self.compiled.as_ref().unwrap().size
	}

	/// Returns the image binary data.
	/// The returned vec will have `width * height * 4` bytes,
	/// in row major format (where `index = (y * width * 4) + (x * 4) + component`).
	pub fn binary(&self) -> &Vec<u8> {
		&self.compiled.as_ref().unwrap().binary
	}

	#[doc(hidden)]
	pub(crate) fn get_compiled(&self) -> &CompiledTexture {
		self.compiled.as_ref().unwrap()
	}
}

/// The metadata about the [`Texture`] asset type.
pub struct TextureMetadata {}

impl TypeMetadata for TextureMetadata {
	fn name(&self) -> asset::TypeId {
		"texture"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		Ok(Box::new(rmp_serde::from_read_ref::<Vec<u8>, Texture>(
			&bin,
		)?))
	}
}

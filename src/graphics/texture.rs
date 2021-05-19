use crate::{
	asset::{self, AssetResult, TypeMetadata},
	math::Vector,
};
use serde::{Deserialize, Serialize};

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
	pub fn set_compiled(&mut self, size: Vector<usize, 2>, binary: Vec<u8>) {
		self.compiled = Some(CompiledTexture { size, binary })
	}

	pub fn size(&self) -> &Vector<usize, 2> {
		&self.compiled.as_ref().unwrap().size
	}

	pub fn binary(&self) -> &Vec<u8> {
		&self.compiled.as_ref().unwrap().binary
	}

	pub(crate) fn get_compiled(&self) -> &CompiledTexture {
		self.compiled.as_ref().unwrap()
	}
}

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

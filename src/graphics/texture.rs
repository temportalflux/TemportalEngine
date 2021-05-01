use crate::{
	asset::{self, AssetResult, TypeMetadata},
	math::Vector,
};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Texture {
	asset_type: String,
	compiled: Option<Compiled>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct Compiled {
	size: Vector<usize, 2>,
	binary: Vec<u8>,
}

impl asset::Asset for Texture {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(TextureMetadata {})
	}
}

impl Texture {
	pub fn set_compiled(&mut self, size: Vector<usize, 2>, binary: Vec<u8>) {
		self.compiled = Some(Compiled { size, binary })
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

use crate::{
	asset::{self},
	math::nalgebra::Vector2,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of [`images`](crate::graphics::image::Image).
#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct Texture {
	asset_type: String,
	compiled: Option<CompiledTexture>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub(crate) struct CompiledTexture {
	pub(crate) size: Vector2<usize>,
	pub(crate) binary: Vec<u8>,
}

impl asset::Asset for Texture {
	fn asset_type() -> asset::TypeId {
		"texture"
	}

	fn decompile(bin: &Vec<u8>) -> anyhow::Result<asset::AnyBox> {
		asset::decompile_asset::<Self>(bin)
	}
}

impl Texture {
	#[doc(hidden)]
	pub fn set_compiled(&mut self, size: Vector2<usize>, binary: Vec<u8>) {
		self.compiled = Some(CompiledTexture { size, binary })
	}

	/// Returns the <width, height> of the image/texture.
	pub fn size(&self) -> &Vector2<usize> {
		&self.compiled.as_ref().unwrap().size
	}

	/// Returns the image binary data.
	/// The returned vec will have `width * height * 4` bytes,
	/// in row major format (where `index = (y * width * 4) + (x * 4) + component`).
	pub fn binary(&self) -> &Vec<u8> {
		&self.compiled.as_ref().unwrap().binary
	}
}

impl crate::asset::kdl::Asset<Texture> for Texture {
	fn kdl_schema() -> kdl_schema::Schema<Texture> {
		use kdl_schema::*;
		Schema {
			nodes: Items::Select(vec![asset::kdl::asset_type::schema::<Texture>(
				|asset, node| {
					asset.asset_type = asset::kdl::asset_type::get(node);
				},
			)]),
			..Default::default()
		}
	}
}

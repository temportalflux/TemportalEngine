use crate::{
	asset::{self},
	graphics::flags::ShaderKind,
};
use serde::{Deserialize, Serialize};

/// The engine asset representation of [`shaders`](crate::graphics::shader::Module).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Shader {
	asset_type: String,

	kind: ShaderKind,

	/// The bytes of the shader data.
	/// When the shader is read from json, these bytes represent the json as a string.
	/// When the shader is read from binary, these bytes represent the compiled shader binary.
	contents: Option<Vec<u8>>,
}

impl Default for Shader {
	fn default() -> Self {
		use asset::Asset;
		Self {
			asset_type: Self::asset_type().to_owned(),
			kind: ShaderKind::Vertex,
			contents: None,
		}
	}
}

impl asset::Asset for Shader {
	fn asset_type() -> asset::TypeId {
		"shader"
	}

	fn decompile(bin: &Vec<u8>) -> anyhow::Result<asset::AnyBox> {
		asset::decompile_asset::<Self>(bin)
	}
}

impl Shader {
	fn set_kind_kdl(&mut self, node: &kdl::KdlNode) {
		self.kind = asset::kdl::typed_enum::get::<ShaderKind>(node);
	}

	/// The shader stage/kind this shader can be used in.
	pub fn kind(&self) -> ShaderKind {
		self.kind
	}

	/// The String or SPIR-V binary of the shader.
	pub fn contents(&self) -> &Vec<u8> {
		&self.contents.as_ref().unwrap()
	}

	#[doc(hidden)]
	pub fn set_contents(&mut self, contents: Vec<u8>) {
		self.contents = Some(contents);
	}

	#[doc(hidden)]
	pub fn content_as_string(&self) -> Result<String, std::string::FromUtf8Error> {
		String::from_utf8(self.contents.as_ref().unwrap().clone())
	}

	#[doc(hidden)]
	pub fn set_contents_from_string(&mut self, contents: String) {
		self.contents = Some(contents.into_bytes());
	}
}

impl asset::kdl::Asset<Shader> for Shader {
	fn kdl_schema() -> kdl_schema::Schema<Shader> {
		use kdl_schema::*;
		Schema {
			nodes: Items::Select(vec![
				asset::kdl::asset_type::schema::<Shader>(|asset, node| {
					asset.asset_type = asset::kdl::asset_type::get(node);
				}),
				asset::kdl::typed_enum::schema::<Shader, ShaderKind>(
					Name::Defined("kind"),
					Shader::set_kind_kdl,
				),
			]),
			..Default::default()
		}
	}
}

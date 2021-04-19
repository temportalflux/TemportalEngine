use crate::asset;
use crate::asset::{AssetResult, TypeMetadata};
use serde::Deserialize;
use serde_json;
use std::path::PathBuf;

#[derive(Deserialize, Debug)]
pub struct Shader {
	/// The bytes of the shader data.
	/// When the shader is read from json, these bytes represent the json as a string.
	/// When the shader is read from binary, these bytes represent the compiled shader binary.
	contents: Option<Vec<u8>>,
}

impl asset::Asset for Shader {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(ShaderMetadata {})
	}
}

impl Shader {
	pub fn set_contents(&mut self, contents: Vec<u8>) {
		self.contents = Some(contents);
	}

	pub fn content_as_string(&self) -> Result<String, std::string::FromUtf8Error> {
		String::from_utf8(self.contents.as_ref().unwrap().clone())
	}

	pub fn set_contents_from_string(&mut self, contents: String) {
		self.contents = Some(contents.into_bytes());
	}
}

pub struct ShaderMetadata {}
impl TypeMetadata for ShaderMetadata {
	fn name(&self) -> asset::TypeName {
		"shader"
	}
	fn read(&self, path: &std::path::Path, json_str: &str) -> AssetResult {
		let mut glsl_path = PathBuf::from(path.parent().unwrap());
		glsl_path.push(path.file_stem().unwrap().to_str().unwrap().to_string() + ".glsl");

		let mut shader: Shader = serde_json::from_str(json_str)?;
		shader.set_contents(std::fs::read(glsl_path)?);
		Ok(Box::new(shader))
	}
}

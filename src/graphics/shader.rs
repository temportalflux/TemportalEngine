use crate::{
	asset::{self, AssetResult, TypeMetadata},
	utility,
};
use serde::{Deserialize, Serialize};
use serde_json;
use std::path::PathBuf;
use temportal_graphics::flags::ShaderKind;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Shader {
	kind: ShaderKind,

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
	fn name(&self) -> asset::TypeId {
		"shader"
	}

	fn read(&self, path: &std::path::Path, json_str: &str) -> AssetResult {
		let mut glsl_path = PathBuf::from(path.parent().unwrap());
		glsl_path.push(path.file_stem().unwrap().to_str().unwrap().to_string() + ".glsl");

		let mut shader: Shader = serde_json::from_str(json_str)?;
		shader.set_contents(std::fs::read(glsl_path)?);
		Ok(Box::new(shader))
	}

	fn compile(
		&self,
		json_path: &std::path::Path,
		asset: &asset::AssetBox,
	) -> Result<Vec<u8>, utility::AnyError> {
		let shader = asset::as_asset::<Shader>(asset);

		let mut compiler = shaderc::Compiler::new().unwrap();
		let mut options = shaderc::CompileOptions::new().unwrap();
		//options.add_macro_definition("EP", Some("main"));
		options.set_generate_debug_info();
		options.set_target_env(
			shaderc::TargetEnv::Vulkan,
			shaderc::EnvVersion::Vulkan1_2 as u32,
		);
		options.set_target_spirv(shaderc::SpirvVersion::V1_5);
		options.set_source_language(shaderc::SourceLanguage::GLSL);

		let shader_code = shader.content_as_string()?;

		let binary = compiler.compile_into_spirv(
			shader_code.as_str(),
			shader.kind.to_shaderc(),
			json_path.file_name().unwrap().to_str().unwrap(),
			"main",
			Some(&options),
		)?;

		let mut shader_out = shader.clone();
		shader_out.set_contents(binary.as_binary_u8().to_vec());

		let bytes = rmp_serde::to_vec(&shader_out)?;
		Ok(bytes)
	}
}

use crate::{
	asset::{deserialize_typed, TypeEditorMetadata},
	engine::{
		asset::{AnyBox, AssetResult},
		graphics,
		utility::AnyError,
	},
};
use std::{
	path::{Path, PathBuf},
	time::SystemTime,
};

pub struct ShaderEditorMetadata {}

impl ShaderEditorMetadata {
	fn glsl_path(&self, path: &Path) -> PathBuf {
		let mut glsl_path = PathBuf::from(path.parent().unwrap());
		glsl_path.push(path.file_stem().unwrap().to_str().unwrap().to_string() + ".glsl");
		glsl_path
	}
}

impl TypeEditorMetadata for ShaderEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(ShaderEditorMetadata {})
	}

	fn last_modified(&self, path: &Path) -> Result<SystemTime, AnyError> {
		let glsl_path = self.glsl_path(&path);
		let asset_last_modified_at = path.metadata()?.modified()?;
		if !glsl_path.exists() {
			return Ok(asset_last_modified_at);
		}
		let glsl_last_modified_at = glsl_path.metadata()?.modified()?;
		Ok(asset_last_modified_at.max(glsl_last_modified_at))
	}

	fn read(&self, path: &std::path::Path, content: &str) -> AssetResult {
		let mut shader = deserialize_typed::<graphics::Shader>(&path, &content)?;
		shader.set_contents(std::fs::read(self.glsl_path(&path))?);
		Ok(Box::new(shader))
	}

	fn compile(&self, json_path: &std::path::Path, asset: AnyBox) -> Result<Vec<u8>, AnyError> {
		let shader = asset.downcast::<graphics::Shader>().unwrap();

		let mut compiler = shaderc::Compiler::new().unwrap();
		let mut options = shaderc::CompileOptions::new().unwrap();
		//options.add_macro_definition("EP", Some("main"));
		options.set_generate_debug_info();
		options.set_target_env(
			shaderc::TargetEnv::Vulkan,
			shaderc::EnvVersion::Vulkan1_2 as u32,
		);
		options.set_target_spirv(shaderc::SpirvVersion::V1_3);
		options.set_source_language(shaderc::SourceLanguage::GLSL);

		let shader_code = shader.content_as_string()?;

		let binary = compiler.compile_into_spirv(
			shader_code.as_str(),
			shader.kind().to_shaderc(),
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

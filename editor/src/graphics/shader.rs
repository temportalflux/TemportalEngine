use engine::task::PinFutureResult;

use crate::{
	asset::{deserialize_typed, BuildPath, EditorOps},
	engine::{
		asset::AnyBox,
		graphics::{self, Shader},
	},
};
use std::path::{Path, PathBuf};

pub struct ShaderEditorOps {}

impl ShaderEditorOps {
	fn glsl_path(path: &Path) -> PathBuf {
		let mut glsl_path = PathBuf::from(path.parent().unwrap());
		glsl_path.push(path.file_stem().unwrap().to_str().unwrap().to_string() + ".glsl");
		glsl_path
	}
}

impl EditorOps for ShaderEditorOps {
	type Asset = Shader;

	fn get_related_paths(path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		Box::pin(async move { Ok(Some(vec![Self::glsl_path(&path)])) })
	}

	fn read(source: PathBuf, file_content: String) -> PinFutureResult<AnyBox> {
		Box::pin(async move {
			let mut shader = deserialize_typed::<Shader>(&source, &file_content)?;
			let external = tokio::fs::read(Self::glsl_path(&source)).await?;
			shader.set_contents(external);
			let boxed: AnyBox = Box::new(shader);
			Ok(boxed)
		})
	}

	fn compile(build_path: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>> {
		Box::pin(async move {
			let shader = asset.downcast::<graphics::Shader>().unwrap();

			let compiler = shaderc::Compiler::new().unwrap();
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
				build_path.file_name(),
				"main",
				Some(&options),
			)?;

			let mut shader_out = shader.clone();
			shader_out.set_contents(binary.as_binary_u8().to_vec());

			let bytes = rmp_serde::to_vec_named(&shader_out)?;
			Ok(bytes)
		})
	}
}

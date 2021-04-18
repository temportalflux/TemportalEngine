use shaderc;
use std::{self, io::Write};

pub use shaderc::ShaderKind;

pub struct BuildContext {
	pub shader: BuildContextShader,
}

pub struct BuildContextShader {
	compiler: shaderc::Compiler,
}

pub type BuildAssetsCallback = fn(ctx: &mut BuildContext) -> Result<(), Box<dyn std::error::Error>>;

impl BuildContextShader {
	pub fn make_options<'a>(&self) -> shaderc::CompileOptions<'a> {
		let mut options = shaderc::CompileOptions::new().unwrap();
		//options.add_macro_definition("EP", Some("main"));
		options.set_generate_debug_info();
		options.set_target_env(
			shaderc::TargetEnv::Vulkan,
			shaderc::EnvVersion::Vulkan1_2 as u32,
		);
		options.set_target_spirv(shaderc::SpirvVersion::V1_5);
		options.set_source_language(shaderc::SourceLanguage::GLSL);
		options
	}
}

pub struct Shader {
	pub name: String,
	pub source: String,
	pub kind: shaderc::ShaderKind,
	pub entry_point: String,
}

pub fn run(build_assets_callback: BuildAssetsCallback) -> Result<(), Box<dyn std::error::Error>> {
	println!("Building assets...");

	let mut context = BuildContext {
		shader: BuildContextShader {
			compiler: shaderc::Compiler::new().unwrap(),
		},
	};

	build_assets_callback(&mut context)
}

pub fn get_output_dir(module: &str) -> Result<std::path::PathBuf, Box<dyn std::error::Error>> {
	let mut workspace_path = std::env::current_dir()?;
	workspace_path.push(module);
	workspace_path.push("src");
	Ok(workspace_path)
}

fn open_or_create(path: &std::path::PathBuf) -> Result<std::fs::File, Box<dyn std::error::Error>> {
	use std::fs::*;
	use std::io::ErrorKind::*;
	match OpenOptions::new().write(true).open(&path) {
		Ok(file) => Ok(file),
		Err(err) => match err.kind() {
			NotFound => match File::create(&path) {
				Ok(file) => Ok(file),
				Err(err) => match err.kind() {
					PermissionDenied => {
						println!("Failed to create, access denied.");
						Err(Box::new(err))
					}
					_ => {
						println!("misc create err");
						Err(Box::new(err))
					}
				},
			},
			PermissionDenied => {
				println!("Failed to open, access denied.");
				Err(Box::new(err))
			}
			_ => {
				println!("misc open err");
				Err(Box::new(err))
			}
		},
	}
}

impl BuildContextShader {
	pub fn compile_into_spirv(
		&mut self,
		options: &shaderc::CompileOptions,
		path_to_output: &std::path::PathBuf,
		shader: Shader,
	) -> Result<(), Box<dyn std::error::Error>> {
		let mut outpath = path_to_output.clone();
		outpath.push(format!("{}.spirv", shader.name));

		println!("Compiling {} into {:?}", shader.name, outpath);

		let binary = self.compiler.compile_into_spirv(
			shader.source.as_str(),
			shader.kind,
			shader.name.as_str(),
			shader.entry_point.as_str(),
			Some(&options),
		)?;

		match open_or_create(&outpath) {
			Ok(mut file) => match file.write_all(binary.as_binary_u8()) {
				Ok(_) => {
					println!("Saved {}.spirv to disk", shader.name);
				}
				Err(err) => {
					println!(
						"Failed to write {}.spriv to disk. Error: {}",
						shader.name, err
					);
				}
			},
			Err(err) => {
				println!(
					"Encountered error opening/creating {}.spirv: {}",
					shader.name, err
				);
			}
		}

		Ok(())
	}
}

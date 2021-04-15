use shaderc;
use std::{self, io::Write};

struct Shader {
	name: String,
	source: String,
	kind: shaderc::ShaderKind,
}

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
	println!("Building assets...");

	let mut compiler = shaderc::Compiler::new().unwrap();
	let options = shaderc::CompileOptions::new().unwrap();
	//options.add_macro_definition("EP", Some("main"));

	compile_into_spirv(
		Shader {
			name: String::from("triangle.vert"),
			source: String::from(include_str!("../triangle.vert")),
			kind: shaderc::ShaderKind::Vertex,
		},
		&mut compiler,
		&options,
	)?;

	compile_into_spirv(
		Shader {
			name: String::from("triangle.frag"),
			source: String::from(include_str!("../triangle.frag")),
			kind: shaderc::ShaderKind::Fragment,
		},
		&mut compiler,
		&options,
	)?;

	Ok(())
}

fn get_output_dir() -> Result<std::path::PathBuf, Box<dyn std::error::Error>> {
	let mut workspace_path = std::env::current_dir()?;
	workspace_path.push("temportal-engine");
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

fn compile_into_spirv(
	shader: Shader,
	compiler: &mut shaderc::Compiler,
	options: &shaderc::CompileOptions,
) -> Result<(), Box<dyn std::error::Error>> {
	let mut outpath = get_output_dir()?;
	outpath.push(format!("{}.spirv", shader.name));

	println!("Compiling {} into {:?}", shader.name, outpath);

	let binary = compiler.compile_into_spirv(
		shader.source.as_str(),
		shader.kind,
		"shader.glsl",
		"main",
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

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
	std::fs::create_dir_all(&path.parent().unwrap())?;
	match std::fs::File::open(&path) {
		Ok(file) => Ok(file),
		Err(err) => match err.kind() {
			std::io::ErrorKind::NotFound => match std::fs::File::create(&path) {
				Ok(file) => Ok(file),
				Err(err) => Err(Box::new(err)),
			},
			_ => Err(Box::new(err)),
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

	let mut file = open_or_create(&outpath)?;
	file.write_all(binary.as_binary_u8())?;

	Ok(())
}

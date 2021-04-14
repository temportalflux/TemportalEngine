use extern_reader;
use std::env;
use std::error::Error;
use std::path::PathBuf;

fn path_to_runtime_dir() -> String {
	format!(
		"{}/../target/debug",
		env::var("CARGO_MANIFEST_DIR").unwrap()
	)
}

fn is_on_any_platform(platforms: Vec<&str>) -> bool {
	let target = env::var("TARGET").unwrap();
	for platform in platforms.iter() {
		if target.contains(platform) {
			return true;
		}
	}
	false
}

fn main() -> Result<(), Box<dyn Error>> {
	println!("cargo:rerun-if-changed=build.rs");

	let target = env::var("TARGET").unwrap();
	if !is_on_any_platform(vec!["pc-windows"]) {
		panic!("Unsuported platform target {:?}", target);
	}

	let architecture = if target.contains("x86_64") {
		"64"
	} else {
		"32"
	};

	let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
	let mut ext_reader = extern_reader::ExternReader::new();
	ext_reader.set_root(Some(format!("{}/../", manifest_dir)));
	let externs = ext_reader.get_externs()?;

	for external in externs {
		let mut dep_path = ext_reader.extern_dir();
		dep_path.push(&external.alias);
		dep_path.push(format!("x{}", architecture));
		println!("cargo:rustc-link-search=all={}", dep_path.display());
		copy_dlls_from_dependency(&dep_path)?;
	}

	Ok(())
}

fn copy_dlls_from_dependency(dep_path: &PathBuf) -> Result<(), Box<dyn Error>> {
	println!("Copying dlls from {:?}", dep_path);
	for entry in std::fs::read_dir(dep_path)? {
		let entry_path = entry.expect("Invalid fs entry").path();
		if let Some(file_name) = entry_path.file_name() {
			let file_name = file_name.to_str().unwrap();
			if file_name.ends_with(".dll") {
				let mut dll_path = PathBuf::from(path_to_runtime_dir());
				dll_path.push(file_name);
				if let Err(msg) = std::fs::copy(&entry_path, dll_path.as_path()) {
					panic!("Failed to copy {:?} to root. {}", entry_path.as_path(), msg);
				}
			}
		}
	}
	Ok(())
}

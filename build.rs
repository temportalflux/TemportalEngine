use std::env;
use std::error::Error;
use std::path::PathBuf;

fn workspace_path_str() -> String {
	format!("{}/..", env::var("CARGO_MANIFEST_DIR").unwrap())
}

fn dependency_path(module: &String, arch: &str) -> PathBuf {
	let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
	PathBuf::from(format!(
		"{}/../externs/{}/x{}",
		manifest_dir, module, arch
	))
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

	let dependencies = [String::from("sdl2")];

	for dep in dependencies.iter() {
		let dep_path = dependency_path(&dep, architecture);
		println!("cargo:rustc-link-search=all={}", dep_path.display());
		copy_dlls_from_dependency(&dep_path)?;
	}

	Ok(())
}

fn copy_dlls_from_dependency(dep_path: &PathBuf) -> Result<(), Box<dyn Error>> {
	for entry in std::fs::read_dir(dep_path)? {
		let entry_path = entry.expect("Invalid fs entry").path();
		if let Some(file_name) = entry_path.file_name() {
			let file_name = file_name.to_str().unwrap();
			if file_name.ends_with(".dll") {
				let mut dll_path = PathBuf::from(workspace_path_str());
				dll_path.push(file_name);
				if let Err(msg) = std::fs::copy(&entry_path, dll_path.as_path()) {
					panic!("Failed to copy {:?} to root. {}", entry_path.as_path(), msg);
				}
			}
		}
	}
	Ok(())
}

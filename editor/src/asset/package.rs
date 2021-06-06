use crate::{
	engine::{asset, utility::VoidResult},
	settings,
};
use std::{self, fs, io::Write, path::PathBuf};
use zip;

pub fn package(
	settings: &settings::Editor,
	module_name: &str,
	module_location: &PathBuf,
	is_editor_pak: bool,
) -> VoidResult {
	let mut output_dir_path = module_location.clone();
	output_dir_path.push("binaries");

	let pak_rel_path = {
		let mut path = PathBuf::new();
		if !is_editor_pak {
			path.push(settings.packager_output());
		}
		path.push("paks");
		path.push(format!("{}.pak", module_name));
		path
	};
	let zip_path = std::env::current_dir().unwrap().join(&pak_rel_path);

	if let Some(parent) = zip_path.parent() {
		if !parent.exists() {
			std::fs::create_dir(&parent)?;
		}
	}

	log::info!(
		target: asset::LOG,
		"[{}] Packaging assets into {}",
		module_name,
		pak_rel_path.display()
	);

	let zip_file = fs::OpenOptions::new()
		.write(true)
		.create(true)
		.truncate(true)
		.open(&zip_path)?;
	let mut zipper = zip::ZipWriter::new(zip_file);
	let zip_options =
		zip::write::FileOptions::default().compression_method(zip::CompressionMethod::BZIP2);

	let files = crate::asset::build::collect_file_paths(&output_dir_path, &Vec::new())?;
	for file_path in files.iter() {
		let relative_path = file_path
			.as_path()
			.strip_prefix(&output_dir_path)?
			.to_str()
			.unwrap();
		let bytes = fs::read(&file_path)?;
		zipper.start_file(relative_path, zip_options)?;
		zipper.write_all(&bytes[..])?;
	}

	zipper.finish()?;

	log::info!(
		target: asset::LOG,
		"[{}] - Packaged {} assets",
		module_name,
		files.len(),
	);

	Ok(())
}

use crate::engine::{asset, utility::VoidResult, Application};
use std::{self, fs, io::Write, path::PathBuf};
use zip;

pub struct Pak {
	/// The name of the pak file. Will be formatted as `{name}.pak`.
	pub name: String,
	/// Where the binaries are that make up this pak.
	pub binaries_directory: PathBuf,
	/// Where the pak file will be put when its created.
	pub output_directory: PathBuf,
}

impl Pak {
	pub fn from_app<T: Application>(pak_output: Option<&str>) -> Self {
		Self {
			name: T::name().to_owned(),
			binaries_directory: PathBuf::from(T::location()).join("binaries"),
			output_directory: {
				let mut path = std::env::current_dir().unwrap();
				if let Some(relative) = pak_output {
					path = path.join(relative);
				}
				path.join("paks")
			},
		}
	}

	pub fn package(&self) -> VoidResult {
		let pak_name = format!("{}.pak", self.name);
		let zip_path = self.output_directory.join(pak_name);

		if let Some(parent) = zip_path.parent() {
			if !parent.exists() {
				std::fs::create_dir_all(&parent)?;
			}
		}

		log::info!(
			target: asset::LOG,
			"[{}] Packaging assets into {}",
			self.name,
			zip_path.display()
		);

		let zip_file = fs::OpenOptions::new()
			.write(true)
			.create(true)
			.truncate(true)
			.open(&zip_path)?;
		let mut zipper = zip::ZipWriter::new(zip_file);
		let zip_options =
			zip::write::FileOptions::default().compression_method(zip::CompressionMethod::BZIP2);

		let files = crate::asset::build::collect_file_paths(&self.binaries_directory, &Vec::new())?;
		for file_path in files.iter() {
			let relative_path = file_path
				.as_path()
				.strip_prefix(&self.binaries_directory)?
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
			self.name,
			files.len(),
		);

		Ok(())
	}
}

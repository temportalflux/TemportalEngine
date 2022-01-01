use engine::{self, asset, Application};
use std::{
	self, fs,
	io::{self},
	path::{Path, PathBuf},
};

#[derive(Debug)]
pub struct Module {
	pub name: String,
	pub assets_directory: PathBuf,
	pub binaries_directory: PathBuf,
}

impl Module {
	pub fn from_app<T: Application>(location: &PathBuf) -> Self {
		Self {
			name: T::name().to_owned(),
			assets_directory: location.join("assets"),
			binaries_directory: location.join("binaries"),
		}
	}

	pub fn build(
		&self,
		asset_manager: &crate::asset::Manager,
		force_build: bool,
	) -> engine::utility::VoidResult {
		if !self.assets_directory.exists() {
			log::info!(
				target: asset::LOG,
				"[{}] Generating assets directory {:?}",
				self.name,
				self.assets_directory
			);
			fs::create_dir(&self.assets_directory)?;
		}

		if !self.binaries_directory.exists() {
			log::info!(
				target: asset::LOG,
				"[{}] Generating output directory {:?}",
				self.name,
				self.binaries_directory
			);
			fs::create_dir(&self.binaries_directory)?;
		} else if force_build {
			log::info!(
				target: asset::LOG,
				"[{}] Wiping output directory {:?}",
				self.name,
				self.binaries_directory
			);
			fs::remove_dir_all(&self.binaries_directory)?;
		}

		let asset_paths = collect_file_paths(&self.assets_directory, &Vec::new())?
			.into_iter()
			.filter(|file_path| {
				let ext = file_path.extension().map(|ext| ext.to_str()).flatten();
				super::SupportedFileTypes::parse_extension(ext).is_some()
			})
			.collect::<Vec<_>>();
		if asset_paths.is_empty() {
			log::info!(target: asset::LOG, "[{}] No assets to build", self.name);
			return Ok(());
		}

		let mut intended_binaries: Vec<PathBuf> = Vec::new();
		let mut dirty_paths = Vec::new();
		let mut skipped_paths = Vec::new();
		let mut failed_asset_paths = Vec::new();
		for asset_file_path in asset_paths.iter() {
			let relative_path = asset_file_path
				.as_path()
				.strip_prefix(&self.assets_directory)?;
			let mut binary_file_path = self.binaries_directory.clone();
			if let Some(parent) = relative_path.parent() {
				binary_file_path.push(parent);
			}
			binary_file_path.push(relative_path.file_stem().unwrap());

			if !binary_file_path.exists()
				|| (asset_manager.last_modified(&asset_file_path)?
					> binary_file_path.metadata()?.modified()?)
			{
				dirty_paths.push((
					asset_file_path.to_owned(),
					relative_path.to_owned(),
					binary_file_path.to_owned(),
				));
			} else {
				skipped_paths.push(relative_path.to_owned());
			}

			intended_binaries.push(binary_file_path);
		}

		log::info!(
			target: asset::LOG,
			"[{}] Building {} assets",
			self.name,
			dirty_paths.len()
		);

		for (asset_file_path, relative_path, binary_file_path) in dirty_paths.into_iter() {
			log::info!(
				target: asset::LOG,
				"[{}] - Building {:?}",
				self.name,
				relative_path
			);
			let (type_id, asset) = match asset_manager.read_sync(&asset_file_path.as_path()) {
				Ok(success) => success,
				Err(err) => {
					log::error!(target: asset::LOG, "{}", err);
					failed_asset_paths.push(asset_file_path.clone());
					continue;
				}
			};
			asset_manager.compile(&asset_file_path, &type_id, asset, &binary_file_path)?;
		}

		if !failed_asset_paths.is_empty() {
			return Err(Box::new(super::Error::FailedToBuild(failed_asset_paths)));
		}

		if !skipped_paths.is_empty() {
			log::info!(
				target: asset::LOG,
				"[{}] - Skipped {} unchanged assets",
				self.name,
				skipped_paths.len()
			);
		}

		let old_binaries = collect_file_paths(&self.binaries_directory, &intended_binaries)?;
		if !old_binaries.is_empty() {
			log::info!(target: asset::LOG, "[{}] Removing old binaries", self.name,);
			for binary_file_path in old_binaries.iter() {
				log::info!(
					target: asset::LOG,
					"[{}] - Deleting old binary {:?}",
					self.name,
					binary_file_path
						.as_path()
						.strip_prefix(&self.binaries_directory)?
				);
				std::fs::remove_file(binary_file_path)?;
			}
		}

		Ok(())
	}
}

pub fn collect_file_paths(path: &Path, ignore: &Vec<PathBuf>) -> io::Result<Vec<PathBuf>> {
	let mut file_paths: Vec<PathBuf> = Vec::new();
	if !path.is_dir() {
		return Ok(file_paths);
	}

	let mut directory_paths: Vec<PathBuf> = vec![path.to_path_buf()];
	while directory_paths.len() > 0 {
		for entry in fs::read_dir(directory_paths.pop().unwrap())? {
			let entry_path = entry?.path().to_path_buf();
			if entry_path.is_dir() {
				directory_paths.push(entry_path);
			} else if !ignore.contains(&entry_path) {
				file_paths.push(entry_path);
			}
		}
	}

	Ok(file_paths)
}

pub fn get_output_dir(module: &str) -> Result<std::path::PathBuf, engine::utility::AnyError> {
	let mut workspace_path = std::env::current_dir()?;
	workspace_path.push(module);
	workspace_path.push("src");
	Ok(workspace_path)
}

use crate::asset::{BuildPath, Manager};
use anyhow::Result;
use engine::{self, asset, Application};
use std::{
	self, fs,
	io::{self},
	path::{Path, PathBuf},
	sync::Arc,
};

#[derive(Debug)]
pub struct Module {
	pub name: String,
	pub assets_directory: PathBuf,
	pub binaries_directory: PathBuf,
}

struct AssetUpdatePlanner {
	source_root: PathBuf,
	destination_root: PathBuf,

	intended_binaries: Vec<PathBuf>,
	dirty_paths: Vec<BuildPath>,
	skipped_paths: Vec<PathBuf>,
}
impl AssetUpdatePlanner {
	fn new(source: PathBuf, destination: PathBuf) -> Self {
		Self {
			source_root: source,
			destination_root: destination,
			intended_binaries: Vec::new(),
			dirty_paths: Vec::new(),
			skipped_paths: Vec::new(),
		}
	}

	fn add_asset_path(
		&mut self,
		source_path: &Path,
		last_modified: std::time::SystemTime,
	) -> anyhow::Result<()> {
		let relative_path = source_path.strip_prefix(&self.source_root)?;

		let mut dst_path = self.destination_root.clone();
		if let Some(parent) = relative_path.parent() {
			dst_path.push(parent);
		}
		dst_path.push(relative_path.file_stem().unwrap());

		if !dst_path.exists() || (last_modified > dst_path.metadata()?.modified()?) {
			self.dirty_paths.push(BuildPath {
				source: source_path.to_owned(),
				relative: relative_path.to_owned(),
				destination: dst_path.to_owned(),
			});
		} else {
			self.skipped_paths.push(relative_path.to_owned());
		}

		self.intended_binaries.push(dst_path);

		Ok(())
	}
}

impl Module {
	pub fn from_app<T: Application>(location: &PathBuf) -> Self {
		Self {
			name: T::name().to_owned(),
			assets_directory: location.join("assets"),
			binaries_directory: location.join("binaries"),
		}
	}

	pub async fn build(
		self: Arc<Self>,
		manager: Arc<Manager>,
		force_build: bool,
	) -> anyhow::Result<()> {
		self.ensure_directories(force_build)?;

		let asset_paths = self.collect_asset_paths()?;
		if asset_paths.is_empty() {
			log::info!(target: asset::LOG, "[{}] No assets to build", self.name);
			return Ok(());
		}

		let mut planner = AssetUpdatePlanner::new(
			self.assets_directory.clone(),
			self.binaries_directory.clone(),
		);

		for asset_file_path in asset_paths.iter() {
			let time = manager.last_modified_at(&asset_file_path).await?;
			planner.add_asset_path(&asset_file_path, time)?;
		}

		log::info!(
			target: asset::LOG,
			"[{}] Building {} assets",
			self.name,
			planner.dirty_paths.len()
		);

		let mut build_tasks = Vec::with_capacity(planner.dirty_paths.len());
		let mut failed_paths: Vec<PathBuf> = Vec::new();
		for build_path in planner.dirty_paths.into_iter() {
			log::info!(
				target: asset::LOG,
				"[{}] Loading {:?}",
				self.name,
				build_path.relative
			);
			let (type_id, asset) = match manager.read(&build_path.source).await {
				Ok(success) => success,
				Err(err) => {
					log::error!(target: asset::LOG, "{}", err);
					failed_paths.push(build_path.source.clone());
					continue;
				}
			};

			let module_name = self.name.clone();
			let async_manager = manager.clone();
			build_tasks.push(engine::task::spawn(asset::LOG.to_owned(), async move {
				log::info!(
					target: asset::LOG,
					"[{}] Building {:?}",
					module_name,
					build_path.relative
				);

				let registration = async_manager.get(&type_id)?;
				let bytes = match registration.compile(build_path.clone(), asset).await {
					Ok(bytes) => bytes,
					Err(err) => {
						log::error!(target: asset::LOG, "[{module_name}] {:?}", err);
						return Ok(());
					}
				};

				fs::create_dir_all(&build_path.destination.parent().unwrap())?;
				fs::write(build_path.destination, bytes)?;

				Ok(())
			}));
		}
		futures::future::join_all(build_tasks.into_iter()).await;

		if !failed_paths.is_empty() {
			return Err(super::Error::FailedToBuild(failed_paths))?;
		}

		if !planner.skipped_paths.is_empty() {
			log::info!(
				target: asset::LOG,
				"[{}] Skipped {} unchanged assets",
				self.name,
				planner.skipped_paths.len()
			);
		}

		let old_binaries =
			collect_file_paths(&self.binaries_directory, &planner.intended_binaries)?;
		if !old_binaries.is_empty() {
			log::info!(target: asset::LOG, "[{}] Removing old binaries", self.name,);
			for binary_file_path in old_binaries.iter() {
				log::info!(
					target: asset::LOG,
					"[{}] Deleting old binary {:?}",
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

	fn ensure_directories(&self, clear_binaries: bool) -> anyhow::Result<()> {
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
		} else if clear_binaries {
			log::info!(
				target: asset::LOG,
				"[{}] Wiping output directory {:?}",
				self.name,
				self.binaries_directory
			);
			fs::remove_dir_all(&self.binaries_directory)?;
		}
		Ok(())
	}

	fn collect_asset_paths(&self) -> anyhow::Result<Vec<PathBuf>> {
		Ok(collect_file_paths(&self.assets_directory, &Vec::new())?
			.into_iter()
			.filter(|file_path| {
				let ext = file_path.extension().map(|ext| ext.to_str()).flatten();
				super::SupportedFileTypes::parse_extension(ext).is_some()
			})
			.collect::<Vec<_>>())
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

pub fn get_output_dir(module: &str) -> Result<std::path::PathBuf> {
	let mut workspace_path = std::env::current_dir()?;
	workspace_path.push(module);
	workspace_path.push("src");
	Ok(workspace_path)
}

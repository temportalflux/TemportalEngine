use crate::{asset::build::collect_file_paths, engine::asset};
use anyhow::Result;
use async_zip::{write::ZipFileWriter, Compression, ZipEntryBuilder};
use std::path::PathBuf;
use tokio::fs;

#[derive(Debug)]
pub struct Pak {
	/// The name of the pak file. Will be formatted as `{name}.pak`.
	pub name: String,
	/// Where the binaries are that make up this pak.
	pub binaries_directory: PathBuf,
	/// Where the pak file will be put when its created.
	pub output_directories: Vec<PathBuf>,
	pub modules: Vec</*module idx*/ usize>,
}

impl Pak {
	pub async fn package(&self) -> Result<()> {
		let pak_name = format!("{}.pak", self.name);
		let zip_paths = self
			.output_directories
			.iter()
			.map(|dir| dir.join(pak_name.clone()))
			.collect::<Vec<_>>();

		for zip_path in zip_paths.iter() {
			if let Some(parent) = zip_path.parent() {
				if !parent.exists() {
					fs::create_dir_all(&parent).await?;
				}
			}
		}

		let files = collect_file_paths(&self.binaries_directory, &Vec::new())?;
		if files.is_empty() {
			log::info!(target: asset::LOG, "[{}] No assets to package", self.name,);
			return Ok(());
		}

		for zip_path in zip_paths.iter() {
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
				.open(&zip_path)
				.await?;

			let mut zipper = ZipFileWriter::new(zip_file);

			for file_path in files.iter() {
				let relative_path = file_path
					.as_path()
					.strip_prefix(&self.binaries_directory)?
					.to_str()
					.unwrap()
					.to_owned();
				let bytes = fs::read(&file_path).await?;
				let options = ZipEntryBuilder::new(relative_path, Compression::Bz);
				zipper.write_entry_whole(options, &bytes[..]).await?;
			}

			zipper.close().await?;

			log::info!(
				target: asset::LOG,
				"[{}] Packaged {} assets",
				self.name,
				files.len(),
			);
		}

		Ok(())
	}
}

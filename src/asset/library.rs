use crate::{asset, utility::VoidResult};
use std::{collections::HashMap, fs};
use zip;

#[derive(Debug)]
struct PakData {
	location: std::path::PathBuf,
	asset_paths: Vec<std::path::PathBuf>,
}

/// Record-keeper of all the assets scanned at runtime.
/// Asset instances are query-able by type or by id.
#[derive(Debug)]
pub struct Library {
	paks: HashMap</*module_name*/ String, PakData>,
	asset_locations: HashMap<asset::Id, asset::Location>,
}

impl Library {
	pub fn new() -> Library {
		Library {
			paks: HashMap::new(),
			asset_locations: HashMap::new(),
		}
	}

	pub fn scan_pak(&mut self, path: &std::path::Path) -> VoidResult {
		let module_name = path.file_stem().unwrap().to_str().unwrap().to_owned();

		if !path.exists() {
			log::warn!(target: asset::LOG, "No such asset pak-age found, skipping {}", path.to_str().unwrap());
			return Ok(());
		}

		let mut pak_data = PakData {
			location: path.to_path_buf(),
			asset_paths: Vec::new(),
		};

		let file = fs::File::open(&path)?;
		let mut archive = zip::ZipArchive::new(file)?;
		for i in 0..archive.len() {
			let item = archive.by_index(i)?;
			let item_path = item.enclosed_name().unwrap();
			pak_data.asset_paths.push(item_path.to_path_buf());
			self.asset_locations.insert(
				asset::Id::new(module_name.as_str(), item_path.to_str().unwrap()),
				asset::Location::from_pak(&pak_data.location, i),
			);
		}

		log::info!(
			target: asset::LOG,
			"Scanned {} assets in {:?}",
			pak_data.asset_paths.len(),
			path.file_name().unwrap()
		);
		self.paks.insert(module_name, pak_data);

		Ok(())
	}

	pub fn find_location(&self, id: &asset::Id) -> Option<asset::Location> {
		self.asset_locations.get(&id).map(|ref_loc| ref_loc.clone())
	}
}

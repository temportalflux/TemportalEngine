use crate::{asset, utility::VoidResult};
use std::{collections::HashMap, fs};
use zip;

#[derive(Debug)]
struct PakData {
	location: std::path::PathBuf,
	asset_paths: Vec<std::path::PathBuf>,
}

#[derive(Debug)]
struct Metadata {
	type_id: asset::TypeIdOwned,
	location: asset::Location,
}

/// Record-keeper of all the assets scanned at runtime.
/// Asset instances are query-able by type or by id.
#[derive(Debug)]
pub struct Library {
	paks: HashMap</*module_name*/ String, PakData>,
	assets: HashMap<asset::Id, Metadata>,
	ids_by_type: HashMap<asset::TypeIdOwned, Vec<asset::Id>>,
}

impl Default for Library {
	fn default() -> Library {
		Library {
			paks: HashMap::new(),
			assets: HashMap::new(),
			ids_by_type: HashMap::new(),
		}
	}
}

impl Library {
	pub fn get() -> &'static std::sync::RwLock<Self> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<Library> = Singleton::uninit();
		unsafe { INSTANCE.get_or_default() }
	}

	/// Returns mutable thread-safe access to the library singleton.
	pub fn write() -> std::sync::RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}

	/// Returns unmutable thread-safe access to the library singleton.
	pub fn read() -> std::sync::RwLockReadGuard<'static, Self> {
		Self::get().read().unwrap()
	}
}

impl Library {

	#[profiling::function]
	pub fn scan_pak_directory(&mut self) -> VoidResult {
		let pak_dir = std::env::current_dir().unwrap().join("paks");
		log::info!(
			target: asset::LOG,
			"Scanning pak directory {}",
			pak_dir.to_str().unwrap()
		);
		for entry in fs::read_dir(pak_dir)? {
			let entry_path = entry?.path().to_path_buf();
			if !entry_path.is_dir() {
				if let Some(ext) = entry_path.extension() {
					if ext == "pak" {
						self.scan_pak(&entry_path)?;
					}
				}
			}
		}

		Ok(())
	}

	/// Scans a specific file at a provided path.
	/// Will emit errors if the path does not exist or is not a `.pak` (i.e. zip) file.
	#[profiling::function]
	pub fn scan_pak(&mut self, path: &std::path::Path) -> VoidResult {
		use std::io::Read;
		let module_name = path.file_stem().unwrap().to_str().unwrap().to_owned();

		if !path.exists() {
			log::warn!(
				target: asset::LOG,
				"Cannot scan \"{}\", no such asset pak-age found.",
				path.to_str().unwrap()
			);
			return Ok(());
		}

		log::info!(
			target: asset::LOG,
			"Scanning asset pak-age \"{}\"",
			path.file_name().unwrap().to_str().unwrap()
		);

		let mut pak_data = PakData {
			location: path.to_path_buf(),
			asset_paths: Vec::new(),
		};

		let file = fs::File::open(&path)?;
		let mut archive = zip::ZipArchive::new(file)?;
		for i in 0..archive.len() {
			let mut item = archive.by_index(i)?;
			let item_path = item.enclosed_name().unwrap().to_path_buf();
			pak_data.asset_paths.push(item_path.clone());

			let type_id = {
				let mut bytes: Vec<u8> = Vec::new();
				item.read_to_end(&mut bytes)?;
				let generic: asset::Generic = rmp_serde::from_read_ref(&bytes)?;
				generic.asset_type.to_owned()
			};

			let id = asset::Id::new(module_name.as_str(), item_path.as_path().to_str().unwrap());
			self.assets.insert(
				id.clone(),
				Metadata {
					type_id: type_id.clone(),
					location: asset::Location::from_pak(&pak_data.location, i),
				},
			);

			if !self.ids_by_type.contains_key(&type_id) {
				self.ids_by_type.insert(type_id.clone(), Vec::new());
			}
			self.ids_by_type.get_mut(&type_id).unwrap().push(id);
		}

		log::info!(
			target: asset::LOG,
			"Scanned {} assets in \"{}\"",
			pak_data.asset_paths.len(),
			path.file_name().unwrap().to_str().unwrap()
		);
		self.paks.insert(module_name, pak_data);

		Ok(())
	}

	/// Returns a list of all ids for a given [`asset type`](asset::Asset).
	/// Returns `None` if the asset type has not been registered or there are no assets
	/// of that type that have been scanned by [`scan_pak`](Library::scan_pak).
	pub fn get_ids_of_type<T: asset::Asset>(&self) -> Option<&Vec<asset::Id>> {
		self.ids_by_type.get(&T::metadata().name().to_owned())
	}

	fn find_asset(&self, id: &asset::Id) -> Option<&Metadata> {
		self.assets.get(id)
	}

	/// Returns true if the provided id has been scanned by [`scan_pak`](Library::scan_pak).
	pub fn has_been_scanned(&self, id: &asset::Id) -> bool {
		self.assets.contains_key(id)
	}

	/// Returns the location of the asset in its pak file.
	pub(crate) fn find_location(&self, id: &asset::Id) -> Option<asset::Location> {
		self.find_asset(id).map(|md| md.location.clone())
	}

	/// Returns the asset type of the id, which can be looked up in the [`Type Registry`](crate::asset::TypeRegistry).
	pub fn get_asset_type(&self, id: &asset::Id) -> Option<asset::TypeIdOwned> {
		self.find_asset(id).map(|md| md.type_id.clone())
	}
}

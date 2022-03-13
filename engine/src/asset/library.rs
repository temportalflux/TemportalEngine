use crate::asset;
use anyhow::Result;
use async_zip::read::fs::ZipFileReader;
use multimap::MultiMap;
use std::{
	collections::HashMap,
	path::{self, Path},
};
use tokio::fs;

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
	ids_by_type: MultiMap<asset::TypeIdOwned, asset::Id>,
}

impl Default for Library {
	fn default() -> Library {
		Library {
			paks: HashMap::new(),
			assets: HashMap::new(),
			ids_by_type: MultiMap::new(),
		}
	}
}

impl Library {
	fn get() -> &'static std::sync::RwLock<Self> {
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
	pub async fn scan_pak_directory() -> Result<()> {
		let pak_dir = std::env::current_dir().unwrap().join("paks");
		log::info!(
			target: asset::LOG,
			"Scanning pak directory {}",
			pak_dir.to_str().unwrap()
		);
		if !pak_dir.exists() {
			return Ok(());
		}
		let mut read_dir = fs::read_dir(pak_dir).await?;
		while let Some(entry) = read_dir.next_entry().await? {
			let entry_path = entry.path().to_path_buf();
			if !entry_path.is_dir() {
				if let Some(ext) = entry_path.extension() {
					if ext == "pak" {
						Self::scan_pak(&entry_path).await?;
					}
				}
			}
		}

		Ok(())
	}

	fn enclose_name<'a>(name: &'a str) -> Option<&'a Path> {
		if name.contains('\0') {
			return None;
		}
		let path = Path::new(name);
		let mut depth = 0usize;
		for component in path.components() {
			match component {
				path::Component::Prefix(_) | path::Component::RootDir => return None,
				path::Component::ParentDir => depth = depth.checked_sub(1)?,
				path::Component::Normal(_) => depth += 1,
				path::Component::CurDir => (),
			}
		}
		Some(path)
	}

	/// Scans a specific file at a provided path.
	/// Will emit errors if the path does not exist or is not a `.pak` (i.e. zip) file.
	pub async fn scan_pak(path: &Path) -> Result<()> {
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

		let archive = ZipFileReader::new(path.to_str().unwrap().to_owned()).await?;
		let entry_count = archive.entries().len();

		let mut assets = HashMap::with_capacity(entry_count);
		let mut ids_by_type = MultiMap::with_capacity(entry_count);

		let mut pak_data = PakData {
			location: path.to_path_buf(),
			asset_paths: Vec::new(),
		};
		for i in 0..entry_count {
			let item = archive.entry_reader(i).await?;
			if item.entry().dir() {
				continue;
			}
			let item_path = Self::enclose_name(item.entry().name()).unwrap().to_owned();
			let id = asset::Id::new(module_name.as_str(), item_path.as_path().to_str().unwrap());
			let type_id = {
				let bytes = item.read_to_end_crc().await?;
				let generic: asset::Generic = rmp_serde::from_read_ref(&bytes)?;
				generic.asset_type.to_owned()
			};

			pak_data.asset_paths.push(item_path);
			assets.insert(
				id.clone(),
				Metadata {
					type_id: type_id.clone(),
					location: asset::Location::from_pak(&pak_data.location, i),
				},
			);

			ids_by_type.insert(type_id.clone(), id);
		}

		log::info!(
			target: asset::LOG,
			"Scanned {} assets in \"{}\"",
			pak_data.asset_paths.len(),
			path.file_name().unwrap().to_str().unwrap()
		);

		{
			let mut library = Self::write();
			for (id, metadata) in assets.into_iter() {
				library.assets.insert(id, metadata);
			}
			for (type_id, ids) in ids_by_type.into_iter() {
				library.ids_by_type.insert_many(type_id, ids.into_iter());
			}
			library.paks.insert(module_name, pak_data);
		}

		Ok(())
	}

	/// Returns a list of all ids for a given [`asset type`](asset::Asset).
	/// Returns `None` if the asset type has not been registered or there are no assets
	/// of that type that have been scanned by [`scan_pak`](Library::scan_pak).
	pub fn get_ids_of_type<T: asset::Asset>(&self) -> Option<&Vec<asset::Id>> {
		self.ids_by_type.get_vec(&T::metadata().name().to_owned())
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

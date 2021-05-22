use crate::{asset, utility::VoidResult, Application};
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
	pub fn get() -> &'static std::sync::RwLock<Library> {
		use crate::utility::singleton::*;
		static mut INSTANCE: Singleton<Library> = Singleton::uninit();
		unsafe { INSTANCE.get() }
	}

	pub fn read() -> std::sync::RwLockReadGuard<'static, Library> {
		Library::get().read().unwrap()
	}
}

impl Library {
	pub fn scan_application<T: Application>() -> VoidResult {
		let mut library = asset::Library::get().write().unwrap();
		library.scan_pak(
			&[T::location(), format!("{}.pak", T::name()).as_str()]
				.iter()
				.collect::<std::path::PathBuf>(),
		)
	}

	#[profiling::function]
	pub fn scan_pak(&mut self, path: &std::path::Path) -> VoidResult {
		use std::io::Read;
		let module_name = path.file_stem().unwrap().to_str().unwrap().to_owned();

		if !path.exists() {
			log::warn!(
				target: asset::LOG,
				"Cannot scan {}, no such asset pak-age found.",
				path.to_str().unwrap()
			);
			return Ok(());
		}

		log::info!(
			target: asset::LOG,
			"Scanning asset pak-age {}",
			path.to_str().unwrap()
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

			self.get_mut_ids_of_type(type_id.clone()).push(id);
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

	pub fn get_ids_of_type<T: asset::Asset>(&self) -> Option<&Vec<asset::Id>> {
		self.ids_by_type.get(&T::metadata().name().to_owned())
	}

	fn get_mut_ids_of_type(&mut self, type_id: asset::TypeIdOwned) -> &mut Vec<asset::Id> {
		if !self.ids_by_type.contains_key(&type_id) {
			self.ids_by_type.insert(type_id.clone(), Vec::new());
		}
		self.ids_by_type.get_mut(&type_id).unwrap()
	}

	fn find_asset(&self, id: &asset::Id) -> Option<&Metadata> {
		self.assets.get(id)
	}

	pub fn find_location(&self, id: &asset::Id) -> Option<asset::Location> {
		self.find_asset(id).map(|md| md.location.clone())
	}

	pub fn get_asset_type(&self, id: &asset::Id) -> Option<asset::TypeIdOwned> {
		self.find_asset(id).map(|md| md.type_id.clone())
	}
}

use crate::{asset, utility::VoidResult, Application};
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

impl Default for Library {
	fn default() -> Library {
		Library {
			paks: HashMap::new(),
			asset_locations: HashMap::new(),
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

	pub fn scan_pak(&mut self, path: &std::path::Path) -> VoidResult {
		optick::event!();
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

use crate::{asset, utility};
use rmp_serde;
use serde::{Deserialize, Serialize};
use std::{path::PathBuf, fs, io::Read};
use zip;

#[derive(Serialize, Deserialize, Debug)]
pub struct AssetGeneric {
	pub asset_type: String,
}

/// Handles the loading of assets by their [`id`](crate::asset::Id),
/// either synchronously or asynchronously.
pub struct Loader {}

impl Loader {
	pub fn new() -> Loader {
		Loader {}
	}

	pub fn decompile(
		&self,
		registry: &asset::TypeRegistry,
		bin_path: &PathBuf,
	) -> asset::AssetResult {
		let bytes = std::fs::read(&bin_path)?;
		let generic: AssetGeneric = rmp_serde::from_read_ref(&bytes)?;
		let type_id = generic.asset_type;
		registry
			.get(type_id.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(
				type_id.to_string(),
			))?
			.decompile(&bytes)
	}

	pub fn load_sync(
		&self,
		registry: &asset::TypeRegistry,
		library: &asset::Library,
		id: &asset::Id,
	) -> Result<asset::AssetBox, utility::AnyError> {

		let location = library.find_location(&id).ok_or(asset::Error::AssetNotFound(id.clone()))?;

		let mut bytes: Vec<u8> = Vec::new();
		{
			let file = fs::File::open(location.pak())?;
			let mut archive = zip::ZipArchive::new(file)?;
			let mut item = archive.by_index(location.index())?;
			let item_path = item.enclosed_name().unwrap();
			item.read_to_end(&mut bytes)?;
		}

		let generic: AssetGeneric = rmp_serde::from_read_ref(&bytes)?;
		let type_id = generic.asset_type;
		registry
			.get(type_id.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(type_id.to_string()))?
			.decompile(&bytes)
	}
}

use crate::{asset, utility};
use rmp_serde;
use serde::{Deserialize, Serialize};
use std::{fs, io::Read, path::PathBuf};
use zip;

#[derive(Serialize, Deserialize, Debug)]
pub struct AssetGeneric {
	pub asset_type: String,
}

/// Handles the loading of assets by their [`id`](crate::asset::Id),
/// either synchronously or asynchronously.
pub struct Loader {}

impl Loader {
	pub fn decompile(bin_path: &PathBuf) -> asset::AssetResult {
		let bytes = std::fs::read(&bin_path)?;
		let generic: AssetGeneric = rmp_serde::from_read_ref(&bytes)?;
		let type_id = generic.asset_type;
		asset::TypeRegistry::read()
			.at(type_id.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(type_id.to_string()))?
			.decompile(&bytes)
	}

	pub fn load_sync(
		library: &asset::Library,
		id: &asset::Id,
	) -> Result<asset::AnyBox, utility::AnyError> {
		let location = library
			.find_location(&id)
			.ok_or(asset::Error::AssetNotFound(id.clone()))?;

		let mut bytes: Vec<u8> = Vec::new();
		{
			let file = fs::File::open(location.pak())?;
			let mut archive = zip::ZipArchive::new(file)?;
			let mut item = archive.by_index(location.index())?;
			item.read_to_end(&mut bytes)?;
		}

		let generic: AssetGeneric = rmp_serde::from_read_ref(&bytes)?;
		let type_id = generic.asset_type;
		asset::TypeRegistry::read()
			.at(type_id.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(type_id.to_string()))?
			.decompile(&bytes)
	}
}

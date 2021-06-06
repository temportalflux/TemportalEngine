use crate::{asset, utility};
use rmp_serde;
use std::{fs, io::Read, path::PathBuf};
use zip;

/// Handles the loading of assets by their [`id`](crate::asset::Id),
/// either synchronously or asynchronously.
pub struct Loader {}

impl Loader {
	pub fn decompile(bin_path: &PathBuf) -> asset::AssetResult {
		let bytes = std::fs::read(&bin_path)?;
		let generic: asset::Generic = rmp_serde::from_read_ref(&bytes)?;
		asset::TypeRegistry::read()
			.at(generic.asset_type.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(
				generic.asset_type.to_string(),
			))?
			.decompile(&bytes)
	}

	pub fn load_sync(id: &asset::Id) -> Result<asset::AnyBox, utility::AnyError> {
		log::info!(
			target: asset::LOG,
			"Synchronously loading '{}'",
			id.as_string()
		);
		let location = asset::Library::read()
			.find_location(&id)
			.ok_or(asset::Error::AssetNotFound(id.clone()))?;

		let mut bytes: Vec<u8> = Vec::new();
		{
			let file = fs::File::open(location.pak())?;
			let mut archive = zip::ZipArchive::new(file)?;
			let mut item = archive.by_index(location.index())?;
			item.read_to_end(&mut bytes)?;
		}

		let generic: asset::Generic = rmp_serde::from_read_ref(&bytes)?;
		asset::TypeRegistry::read()
			.at(generic.asset_type.as_str())
			.ok_or(asset::Error::UnregisteredAssetType(
				generic.asset_type.to_string(),
			))?
			.decompile(&bytes)
	}
}

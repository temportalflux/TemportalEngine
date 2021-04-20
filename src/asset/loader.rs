use crate::asset;
use rmp_serde;
use serde::{Deserialize, Serialize};
use std::path::PathBuf;

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
				bin_path.clone(),
				type_id.to_string(),
			))?
			.decompile(&bytes)
	}
}

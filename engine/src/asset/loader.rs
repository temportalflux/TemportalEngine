use crate::asset::{self, AssetNotFound, UnregisteredAssetType};
use anyhow::Result;
use rmp_serde;
use std::io::Read;

/// Handles the loading of assets by their [`id`](crate::asset::Id),
/// either synchronously or asynchronously.
pub struct Loader;

impl Loader {
	#[profiling::function]
	pub fn load_sync(id: &asset::Id) -> Result<asset::AnyBox> {
		let location = Self::get_location(id)?;
		let bytes = Self::read_from_location_sync(&location)?;
		Self::decompile_asset(bytes)
	}

	pub async fn load(id: &asset::Id) -> Result<asset::AnyBox> {
		let location = Self::get_location(id)?;
		let bytes = Self::read_from_location_async(&location).await?;
		Self::decompile_asset(bytes)
	}

	#[profiling::function]
	fn get_location(id: &asset::Id) -> Result<asset::Location, AssetNotFound> {
		asset::Library::read()
			.find_location(&id)
			.ok_or(AssetNotFound(id.clone()))
	}

	#[profiling::function]
	fn read_from_location_sync(location: &asset::Location) -> Result<Vec<u8>> {
		let file = std::fs::File::open(location.pak())?;
		let mut archive = zip::ZipArchive::new(file)?;
		let mut item = archive.by_index(location.index())?;

		let mut bytes: Vec<u8> = Vec::new();
		item.read_to_end(&mut bytes)?;
		Ok(bytes)
	}

	async fn read_from_location_async(location: &asset::Location) -> Result<Vec<u8>> {
		use async_zip::read::fs::ZipFileReader;
		let pak_path = location.pak().to_str().unwrap().to_owned();
		let archive = ZipFileReader::new(pak_path).await?;
		let item = archive.entry_reader(location.index()).await?;

		let bytes = item.read_to_end_crc().await?;
		Ok(bytes)
	}

	#[profiling::function]
	fn decompile_asset(bytes: Vec<u8>) -> Result<asset::AnyBox> {
		let generic: asset::Generic = rmp_serde::from_read_ref(&bytes)?;
		asset::TypeRegistry::read()
			.at(generic.asset_type.as_str())
			.ok_or(UnregisteredAssetType(generic.asset_type.to_string()))?
			.decompile(&bytes)
	}
}

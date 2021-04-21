use crate::asset;

#[derive(Debug)]
pub enum Error {
	UnregisteredAssetType(String),
	AssetNotFound(asset::Id),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::UnregisteredAssetType(ref type_id) => {
				write!(f, "Asset type id {} has not been registered", type_id)
			}
			Error::AssetNotFound(ref id) => write!(f, "No such asset {:?} found", id),
		}
	}
}

impl std::error::Error for Error {}

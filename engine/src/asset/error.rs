use crate::asset;

#[derive(Debug)]
pub enum Error {
	UnregisteredAssetType(String),
	AssetNotFound(asset::Id),
	ExtensionNotSupported(Option<String>),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Self::UnregisteredAssetType(ref type_id) => {
				write!(f, "Asset type id {} has not been registered", type_id)
			}
			Self::AssetNotFound(ref id) => write!(f, "No such asset {:?} found", id),
			Self::ExtensionNotSupported(ref ext) => {
				write!(f, "File extension {:?} is not supported", ext)
			}
		}
	}
}

impl std::error::Error for Error {}

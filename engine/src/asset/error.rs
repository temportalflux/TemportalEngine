use crate::asset;

#[derive(Debug)]
pub enum Error {
	ExtensionNotSupported(Option<String>),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Self::ExtensionNotSupported(ref ext) => {
				write!(f, "File extension {:?} is not supported", ext)
			}
		}
	}
}

impl std::error::Error for Error {}

#[derive(thiserror::Error, Debug)]
pub struct AssetNotFound(pub asset::Id);
impl std::fmt::Display for AssetNotFound {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Asset {:?} does not exist.", self.0)
	}
}

#[derive(thiserror::Error, Debug)]
pub struct UnregisteredAssetType(pub String);
impl std::fmt::Display for UnregisteredAssetType {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Asset type id {} has not been registered", self.0)
	}
}

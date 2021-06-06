#[derive(Debug)]
pub enum Error {
	InvalidJson(std::path::PathBuf, String),
	MissingTypeId(std::path::PathBuf),
	UnregisteredAssetType(std::path::PathBuf, String),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
			Error::InvalidJson(ref path, ref msg) => {
				write!(f, "Asset {} {}", path.to_str().unwrap(), msg)
			}
			Error::MissingTypeId(ref path) => {
				write!(f, "Asset {} is missing a type id", path.to_str().unwrap())
			}
			Error::UnregisteredAssetType(ref path, ref type_id) => write!(
				f,
				"Asset type id {} in {} has not been registered",
				type_id,
				path.to_str().unwrap()
			),
		}
	}
}

impl std::error::Error for Error {}

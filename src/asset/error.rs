#[derive(Debug)]
pub enum Error {
	UnregisteredAssetType(std::path::PathBuf, String),
}

pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match *self {
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

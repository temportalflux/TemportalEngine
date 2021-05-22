/// A unique identifier given to each instance of a class which implements [`Asset`](crate::asset::Asset).
#[derive(Debug, Hash, Clone)]
pub struct Id {
	module_name: String,
	asset_path: std::path::PathBuf,
}

impl Id {
	pub fn new(module_name: &str, asset_path: &str) -> Id {
		Id {
			module_name: module_name.to_string(),
			asset_path: std::path::PathBuf::from(asset_path),
		}
	}

	pub fn file_name(&self) -> String {
		self.asset_path
			.file_name()
			.unwrap()
			.to_str()
			.unwrap()
			.to_owned()
	}

	pub fn to_str(&self) -> &str {
		self.asset_path.to_str().unwrap()
	}

	pub fn name(&self) -> String {
		self.to_str().to_owned().replace("\\", "/")
	}

	pub fn short_id(&self) -> String {
		format!("{}:{}", self.module_name, self.name())
	}
}

impl PartialEq for Id {
	fn eq(&self, other: &Self) -> bool {
		self.module_name == other.module_name && self.asset_path == other.asset_path
	}
}

impl std::cmp::Eq for Id {}

#[derive(Debug, Clone)]
pub struct Location {
	pak_path: std::path::PathBuf,
	pak_index: usize,
}

impl Location {
	pub fn from_pak(pak_path: &std::path::Path, index: usize) -> Location {
		Location {
			pak_path: pak_path.to_path_buf(),
			pak_index: index,
		}
	}

	pub fn pak(&self) -> &std::path::Path {
		&self.pak_path
	}

	pub fn index(&self) -> usize {
		self.pak_index
	}
}

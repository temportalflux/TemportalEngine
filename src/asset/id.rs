use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::fmt;

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

	pub fn from_short_id(short_id: &str) -> Option<Self> {
		let parts = short_id.split(':').collect::<Vec<_>>();
		if parts.len() == 1 {
			return None;
		}
		Some(Self::new(&parts[0], &parts[1]))
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

impl std::fmt::Display for Id {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "{}", self.short_id())
	}
}

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

impl Serialize for Id {
	fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
	where
		S: Serializer,
	{
		serializer.serialize_str(&self.short_id())
	}
}

struct IdVisitor;
impl<'de> de::Visitor<'de> for IdVisitor {
	type Value = Id;

	fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
		formatter.write_str("an integer between -2^31 and 2^31")
	}

	fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
	where
		E: de::Error,
	{
		Id::from_short_id(value).ok_or(de::Error::custom("invalid asset id"))
	}
}

impl<'de> Deserialize<'de> for Id {
	fn deserialize<D>(deserializer: D) -> Result<Id, D::Error>
	where
		D: Deserializer<'de>,
	{
		deserializer.deserialize_str(IdVisitor)
	}
}

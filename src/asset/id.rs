use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::fmt;

/// A unique identifier given to each instance of a class which implements [`Asset`](crate::asset::Asset).
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Id {
	module_name: String,
	asset_path: std::path::PathBuf,
}

impl Id {
	/// Creates an asset id based on the name of a module and
	/// the path to the asset from that module's `assets` directory.
	///
	/// It is recommended that modules use
	/// [`Application::get_asset_id`](crate::Application::get_asset_id)
	/// to get asset ids by path.
	pub fn new(module_name: &str, asset_path: &str) -> Id {
		Id {
			module_name: module_name.to_string(),
			asset_path: std::path::PathBuf::from(asset_path),
		}
	}

	/// Returns the full name of the asset (i.e. its path within a module),
	/// using forward-slash separators `/`.
	pub fn name(&self) -> String {
		self.asset_path
			.to_str()
			.unwrap()
			.to_owned()
			.replace("\\", "/")
	}

	/// Returns the stringified eqivalent of the id,
	/// concatenating the module name and path with the format:
	/// `{module_name}:{path_name}`.
	///
	/// Use `Id::try_from(&str)` to convert from the string back into an Id.
	pub fn as_string(&self) -> String {
		format!("{}:{}", self.module_name, self.name())
	}

	/// Returns true if this asset id has been scanned
	/// by the asset [`Library`](crate::asset::Library).
	pub fn has_been_scanned(&self) -> bool {
		crate::asset::Library::read().has_been_scanned(self)
	}
}

impl std::convert::TryFrom<&str> for Id {
	type Error = ();

	/// Parses an asset id from a string created by [`as_string`](Id::as_string).
	///
	/// Examples:
	/// ```
	/// let id = Id::try_from("invalid");
	/// assert_eq!(id, Err(()));
	/// ```
	/// ```
	/// let id = Id::try_from("somemodule:path/to/asset");
	/// assert_eq!(id, Ok(Id::new("somemodule", "path/to/asset")));
	/// ```
	fn try_from(s: &str) -> Result<Self, Self::Error> {
		let parts = s.split(':').collect::<Vec<_>>();
		if parts.len() == 1 {
			return Err(());
		}
		Ok(Self::new(&parts[0], &parts[1]))
	}
}

impl std::fmt::Display for Id {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "{}", self.as_string())
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
		serializer.serialize_str(&self.as_string())
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
		use std::convert::TryFrom;
		Id::try_from(value).map_err(|_| de::Error::custom("invalid asset id"))
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

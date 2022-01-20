use crate::utility::Result;
use std::{fs, path::PathBuf};

/// Helper trait to aid in the creation of settings/config files that are loaded at runtime.
pub trait SaveData<TSelf: Default> {
	/// The path from the current working directory to read/write the config to.
	fn path() -> &'static str;

	/// Deserialize from a json string.
	/// More than likely you'll want the boilderplate:
	/// ```
	/// fn from_json(json: &str) -> Result<Self> {
	/// 	let value: Self = serde_json::from_str(json)?;
	/// 	Ok(value)
	/// }
	/// ```
	fn from_json(json: &str) -> Result<TSelf>;

	/// Serialize to a json string.
	/// More than likely you'll want the boilderplate:
	/// ```
	/// fn to_json(&self) -> Result<String> {
	/// 	let json = serde_json::to_string_pretty(self)?;
	/// 	Ok(json)
	/// }
	/// ```
	fn to_json(&self) -> Result<String>;

	/// Constructs the absolutely path to the serialized file.
	fn path_buf() -> PathBuf {
		let cwd = std::env::current_dir().unwrap();
		[cwd.to_str().unwrap(), Self::path()]
			.iter()
			.collect::<PathBuf>()
	}

	/// Loads the save data from file.
	fn load() -> Result<TSelf> {
		match fs::read_to_string(&Self::path_buf()) {
			Ok(json) => Self::from_json(json.as_str()),
			Err(e) => match e.kind() {
				std::io::ErrorKind::NotFound => Ok(Default::default()),
				_ => Err(e)?,
			},
		}
	}

	/// Save the save data to file.
	fn save(&self) -> Result<()> {
		let path = Self::path_buf();
		fs::create_dir_all(path.parent().unwrap())?;
		fs::write(&path, self.to_json()?)?;
		Ok(())
	}
}

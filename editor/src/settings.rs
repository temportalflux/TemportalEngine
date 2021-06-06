use crate::engine::utility::{AnyError, VoidResult};
use serde::{Deserialize, Serialize};
use std::{fs, path::PathBuf};

pub trait Settings<TSelf: Default> {
	fn name() -> &'static str;
	fn from_json(json: &str) -> Result<TSelf, AnyError>;
	fn to_json(&self) -> Result<String, AnyError>;
	fn path() -> PathBuf {
		let cwd = std::env::current_dir().unwrap();
		[cwd.to_str().unwrap(), "config", Self::name()]
			.iter()
			.collect::<PathBuf>()
	}
	fn load() -> Result<TSelf, AnyError> {
		match fs::read_to_string(&Self::path()) {
			Ok(json) => Self::from_json(json.as_str()),
			Err(e) => match e.kind() {
				std::io::ErrorKind::NotFound => Ok(Default::default()),
				_ => Err(Box::new(e)),
			},
		}
	}
	fn save(&self) -> VoidResult {
		let path = Self::path();
		fs::create_dir_all(path.parent().unwrap())?;
		fs::write(&path, self.to_json()?)?;
		Ok(())
	}
}

#[derive(Deserialize, Serialize, Debug)]
pub struct Editor {
	open_windows: Vec<String>,
}

impl Default for Editor {
	fn default() -> Editor {
		Editor {
			open_windows: Vec::new(),
		}
	}
}

impl Settings<Editor> for Editor {
	fn name() -> &'static str {
		"editor.json"
	}
	fn from_json(json: &str) -> Result<Editor, AnyError> {
		let value: Editor = serde_json::from_str(json)?;
		Ok(value)
	}
	fn to_json(&self) -> Result<String, AnyError> {
		let json = serde_json::to_string_pretty(self)?;
		Ok(json)
	}
}

impl Editor {
	pub fn is_window_open(&self, id: &String) -> bool {
		self.open_windows.contains(id)
	}

	pub fn set_window_open(&mut self, id: &String, is_open: bool) {
		if is_open {
			if !self.is_window_open(id) {
				self.open_windows.push(id.clone());
			}
		} else {
			self.open_windows.retain(|win_id| win_id != id);
		}
	}
}

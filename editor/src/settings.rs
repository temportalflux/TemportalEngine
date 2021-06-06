use crate::engine::utility::{AnyError, SaveData};
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize, Debug)]
pub struct Editor {
	open_windows: Vec<String>,
	/// The output directory (relative to the current working directory of the editor runtime),
	/// that the [`packager`](`crate::asset::package`) should place `.pak` files.
	#[serde(default = "default_pak_output")]
	packager_output: String,
}

impl Default for Editor {
	fn default() -> Editor {
		Editor {
			open_windows: Vec::new(),
			packager_output: default_pak_output(),
		}
	}
}

fn default_pak_output() -> String {
	"paks".to_owned()
}

impl SaveData<Editor> for Editor {
	fn path() -> &'static str {
		"config/editor.json"
	}
	fn from_json(json: &str) -> Result<Self, AnyError> {
		let value: Self = serde_json::from_str(json)?;
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

	pub fn packager_output(&self) -> &String {
		&self.packager_output
	}
}

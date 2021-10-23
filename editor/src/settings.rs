use engine::utility::{AnyError, SaveData};
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize, Debug)]
pub struct Editor {
	#[serde(default)]
	open_windows: Vec<String>,
}

impl Default for Editor {
	fn default() -> Editor {
		Editor {
			open_windows: Vec::new(),
		}
	}
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
		let mut json = serde_json::to_string_pretty(self)?;
		json = json.replace("  ", "\t");
		Ok(json)
	}
}

impl Editor {
	pub fn get_open_window_list(&self) -> &Vec<String> {
		&self.open_windows
	}

	pub fn set_window_open(&mut self, id: &String, is_open: bool) {
		if is_open {
			if !self.open_windows.contains(id) {
				self.open_windows.push(id.clone());
			}
		} else {
			self.open_windows.retain(|win_id| win_id != id);
		}
	}
}

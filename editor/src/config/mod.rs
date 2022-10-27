use serde::{Deserialize, Serialize};

mod crates;
pub use crates::*;

#[derive(Default, Serialize, Deserialize)]
pub struct PersistentData {
	#[serde(default, skip_serializing_if = "Option::is_none")]
	pub egui: Option<egui::Memory>,
}
impl PersistentData {
	pub async fn read_from_disk() -> Self {
		let json = tokio::fs::read_to_string("persistent_data.json").await.ok();
		json.map(|json| serde_json::from_str(&json).ok())
			.flatten()
			.unwrap_or_default()
	}

	pub fn write_to_disk(&self) -> anyhow::Result<()> {
		let json = serde_json::to_string_pretty(&self)?;
		std::fs::write("persistent_data.json", json)?;
		Ok(())
	}
}

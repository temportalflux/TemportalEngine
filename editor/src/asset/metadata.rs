use anyhow::Result;
use engine::asset::{AnyBox, AssetResult};
use std::{path::Path, time::SystemTime};

pub trait TypeEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata>
	where
		Self: Sized;
	fn last_modified(&self, path: &Path) -> Result<SystemTime> {
		Ok(path.metadata()?.modified()?)
	}
	fn read(&self, path: &Path, json_str: &str) -> AssetResult;
	fn compile(&self, json_path: &Path, asset: AnyBox) -> Result<Vec<u8>>;
}

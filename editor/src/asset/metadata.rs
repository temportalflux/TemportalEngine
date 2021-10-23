use engine::{
	asset::{AnyBox, AssetResult},
	utility::AnyError,
};
use std::{path::Path, time::SystemTime};

pub trait TypeEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata>
	where
		Self: Sized;
	fn last_modified(&self, path: &Path) -> Result<SystemTime, AnyError> {
		Ok(path.metadata()?.modified()?)
	}
	fn read(&self, path: &Path, json_str: &str) -> AssetResult;
	fn compile(&self, json_path: &Path, asset: AnyBox) -> Result<Vec<u8>, AnyError>;
}

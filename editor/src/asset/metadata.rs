use crate::asset::BuildPath;
use anyhow::Result;
use engine::{
	asset::{AnyBox, AssetResult},
	task::PinFutureResultLifetime,
};
use std::{path::Path, time::SystemTime};

pub trait TypeEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata + 'static + Send + Sync>
	where
		Self: Sized;
	fn last_modified(&self, path: &Path) -> Result<SystemTime> {
		Ok(path.metadata()?.modified()?)
	}
	fn read(&self, path: &Path, json_str: &str) -> AssetResult;

	fn compile<'a>(
		&'a self,
		build_path: &'a BuildPath,
		asset: AnyBox,
	) -> PinFutureResultLifetime<'a, Vec<u8>>;
}

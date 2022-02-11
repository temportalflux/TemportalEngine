use crate::{
	asset::TypeEditorMetadata,
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::render_pass::Pass,
	},
};
use anyhow::Result;
use std::path::Path;

pub struct PassEditorMetadata;
impl TypeEditorMetadata for PassEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(PassEditorMetadata {})
	}

	fn read(&self, path: &Path, content: &str) -> AssetResult {
		crate::asset::deserialize::<Pass>(&path, &content)
	}

	fn compile(&self, _: &Path, asset: AnyBox) -> Result<Vec<u8>> {
		Ok(rmp_serde::to_vec(&asset.downcast::<Pass>().unwrap())?)
	}
}

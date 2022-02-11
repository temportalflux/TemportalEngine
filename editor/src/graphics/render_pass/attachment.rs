use crate::{
	asset::TypeEditorMetadata,
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::render_pass::Attachment,
	},
};
use anyhow::Result;
use serde_json;
use std::path::Path;

pub struct AttachmentEditorMetadata;
impl TypeEditorMetadata for AttachmentEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(AttachmentEditorMetadata {})
	}

	fn read(&self, _path: &Path, json_str: &str) -> AssetResult {
		Ok(Box::new(serde_json::from_str::<Attachment>(json_str)?))
	}

	fn compile(&self, _: &Path, asset: AnyBox) -> Result<Vec<u8>> {
		Ok(rmp_serde::to_vec(&asset.downcast::<Attachment>().unwrap())?)
	}
}

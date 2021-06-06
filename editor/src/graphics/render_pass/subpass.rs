use crate::{
	asset::TypeEditorMetadata,
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::render_pass::Subpass,
		utility::AnyError,
	},
};
use serde_json;
use std::path::Path;

pub struct SubpassEditorMetadata;
impl TypeEditorMetadata for SubpassEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(SubpassEditorMetadata {})
	}

	fn read(&self, _path: &Path, json_str: &str) -> AssetResult {
		Ok(Box::new(serde_json::from_str::<Subpass>(json_str)?))
	}

	fn compile(&self, _: &Path, asset: AnyBox) -> Result<Vec<u8>, AnyError> {
		Ok(rmp_serde::to_vec(&asset.downcast::<Subpass>().unwrap())?)
	}
}

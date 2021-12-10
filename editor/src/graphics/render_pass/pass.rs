use crate::{
	asset::TypeEditorMetadata,
	engine::{
		asset::{AnyBox, AssetResult},
		graphics::render_pass::Pass,
		utility::AnyError,
	},
};
use serde_json;
use std::path::Path;

pub struct PassEditorMetadata;
impl TypeEditorMetadata for PassEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(PassEditorMetadata {})
	}

	fn read(&self, path: &Path, content: &str) -> AssetResult {
		use crate::asset::SupportedFileTypes;
		let ext = path.extension().map(|ext| ext.to_str()).flatten();
		match SupportedFileTypes::parse_extension(ext) {
			Some(SupportedFileTypes::Json) => Ok(Box::new(serde_json::from_str::<Pass>(content)?)),
			Some(SupportedFileTypes::Kdl) => {
				Ok(Box::new(Pass::kdl_schema().parse_and_validate(&content)?))
			}
			_ => Err(Box::new(engine::asset::Error::ExtensionNotSupported(
				ext.map(|ext| ext.to_owned()),
			))),
		}
	}

	fn compile(&self, _: &Path, asset: AnyBox) -> Result<Vec<u8>, AnyError> {
		Ok(rmp_serde::to_vec(&asset.downcast::<Pass>().unwrap())?)
	}
}

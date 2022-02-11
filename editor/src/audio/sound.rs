use crate::{
	asset::TypeEditorMetadata,
	engine::{
		asset::{AnyBox, AssetResult},
		audio::{Sound, SourceKind},
	},
};
use anyhow::Result;
use serde_json;
use std::path::{Path, PathBuf};

pub struct SoundEditorMetadata;

impl SoundEditorMetadata {
	fn source_file_path(json_path: &Path, kind: SourceKind) -> PathBuf {
		let mut png_path = json_path.parent().unwrap().to_path_buf();
		png_path.push(json_path.file_stem().unwrap());
		png_path.set_extension(kind.extension());
		png_path
	}
}

impl TypeEditorMetadata for SoundEditorMetadata {
	fn boxed() -> Box<dyn TypeEditorMetadata> {
		Box::new(SoundEditorMetadata {})
	}

	fn read(&self, path: &Path, json_str: &str) -> AssetResult {
		let mut sound = serde_json::from_str::<Sound>(json_str)?;
		sound.set_binary(std::fs::read(Self::source_file_path(path, sound.kind()))?);
		Ok(Box::new(sound))
	}

	fn compile(&self, _: &Path, asset: AnyBox) -> Result<Vec<u8>> {
		Ok(rmp_serde::to_vec(&asset.downcast::<Sound>().unwrap())?)
	}
}

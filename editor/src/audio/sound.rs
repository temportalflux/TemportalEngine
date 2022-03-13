use crate::{
	asset::{BuildPath, TypeEditorMetadata},
	engine::{
		asset::{AnyBox, AssetResult},
		audio::{Sound, SourceKind},
	},
};
use engine::task::PinFutureResultLifetime;
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
	fn boxed() -> Box<dyn TypeEditorMetadata + 'static + Send + Sync> {
		Box::new(SoundEditorMetadata {})
	}

	fn read(&self, path: &Path, json_str: &str) -> AssetResult {
		let mut sound = serde_json::from_str::<Sound>(json_str)?;
		sound.set_binary(std::fs::read(Self::source_file_path(path, sound.kind()))?);
		Ok(Box::new(sound))
	}

	fn compile<'a>(
		&'a self,
		_build_path: &'a BuildPath,
		asset: AnyBox,
	) -> PinFutureResultLifetime<'a, Vec<u8>> {
		Box::pin(async move { Ok(rmp_serde::to_vec(&asset.downcast::<Sound>().unwrap())?) })
	}
}

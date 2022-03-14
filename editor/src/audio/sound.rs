use crate::{
	asset::{BuildPath, EditorOps},
	engine::{
		asset::AnyBox,
		audio::{Sound, SourceKind},
	},
};
use engine::task::PinFutureResult;
use serde_json;
use std::path::{Path, PathBuf};

pub struct SoundEditorOps;
impl SoundEditorOps {
	fn source_file_path(source: &Path, kind: SourceKind) -> PathBuf {
		let mut png_path = source.parent().unwrap().to_path_buf();
		png_path.push(source.file_stem().unwrap());
		png_path.set_extension(kind.extension());
		png_path
	}
}
impl EditorOps for SoundEditorOps {
	type Asset = Sound;

	fn get_related_paths(path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		Box::pin(async move {
			let file_content = tokio::fs::read_to_string(&path).await?;
			let sound = serde_json::from_str::<Sound>(&file_content)?;
			Ok(Some(vec![Self::source_file_path(&path, sound.kind())]))
		})
	}

	fn read(source: PathBuf, file_content: String) -> PinFutureResult<AnyBox> {
		Box::pin(async move {
			let mut sound = serde_json::from_str::<Sound>(&file_content)?;
			let external_path = Self::source_file_path(&source, sound.kind());
			sound.set_binary(std::fs::read(external_path)?);
			let boxed: AnyBox = Box::new(sound);
			Ok(boxed)
		})
	}

	fn compile(_: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>> {
		Box::pin(async move { Ok(rmp_serde::to_vec(&asset.downcast::<Sound>().unwrap())?) })
	}
}

use crate::asset::BuildPath;
use engine::{
	asset::{AnyBox, Asset},
	task::PinFutureResult,
};
use std::path::PathBuf;

pub trait EditorOps {
	type Asset: Asset;

	fn get_related_paths(_path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		Box::pin(async move { Ok(None) })
	}

	fn read(source: PathBuf, file_content: String) -> PinFutureResult<AnyBox>;

	fn compile(build_path: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>>;
}

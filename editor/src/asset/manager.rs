use crate::{
	asset,
};
use engine::{
	self,
	asset::{AnyBox, Generic, TypeId},
	utility::AnyError,
};
use serde_json;
use std::{
	collections::HashMap,
	fs,
	path::{Path, PathBuf},
	time::SystemTime,
};

type EditorMetadataBox = Box<dyn asset::TypeEditorMetadata>;

/// Handles creating, saving, loading, moving, and deleting an asset at a given path.
/// Only accessible during editor-runtime whereas [Loader](engine::asset::Loader)
/// handles loading built assets during game-runtime.
pub struct Manager {
	editor_metadata: HashMap<TypeId, EditorMetadataBox>,
}

impl Manager {
	pub fn new() -> Manager {
		Manager {
			editor_metadata: HashMap::new(),
		}
	}

	pub fn register<TAsset, TEditorMetadata>(&mut self)
	where
		TAsset: engine::asset::Asset,
		TEditorMetadata: asset::TypeEditorMetadata,
	{
		let runtime_metadata = TAsset::metadata();
		if !self.editor_metadata.contains_key(runtime_metadata.name()) {
			self.editor_metadata
				.insert(runtime_metadata.name(), TEditorMetadata::boxed());
			log::info!(
				target: engine::asset::LOG,
				"Registering asset type editor metadata \"{}\"",
				runtime_metadata.name()
			);
		} else {
			log::error!(
				target: engine::asset::LOG,
				"Encountered duplicate asset type with editor metadata \"{}\"",
				runtime_metadata.name()
			);
		}
	}

	fn metadata<'r>(&self, type_id: &'r str) -> Result<&EditorMetadataBox, AnyError> {
		let metadata = self.editor_metadata.get(type_id).ok_or(
			engine::asset::Error::UnregisteredAssetType(type_id.to_string()),
		)?;
		Ok(metadata)
	}

	fn read_type_id_sync(&self, path: &Path) -> Result<String, AnyError> {
		let absolute_path = path.canonicalize()?;
		let file_json = fs::read_to_string(&absolute_path)?;
		let type_id = Manager::read_asset_type(file_json.as_str())?;
		Ok(type_id)
	}

	pub fn last_modified(&self, path: &Path) -> Result<SystemTime, AnyError> {
		let type_id = self.read_type_id_sync(path)?;
		let metadata = self.metadata(&type_id)?;
		metadata.last_modified(&path)
	}

	/// Synchronously reads an asset json from a provided path, returning relevant asset loading errors.
	pub fn read_sync(&self, path: &Path) -> Result<(String, AnyBox), AnyError> {
		let absolute_path = path.canonicalize()?;
		let file_json = fs::read_to_string(&absolute_path)?;
		let type_id = Manager::read_asset_type(file_json.as_str())?;
		let asset = self
			.editor_metadata
			.get(type_id.as_str())
			.ok_or(engine::asset::Error::UnregisteredAssetType(
				type_id.to_string(),
			))?
			.read(&absolute_path, file_json.as_str())?;
		Ok((type_id, asset))
	}

	fn read_asset_type(json_str: &str) -> Result<String, AnyError> {
		let generic: Generic = serde_json::from_str(json_str)?;
		return Ok(generic.asset_type);
	}

	pub fn compile(
		&self,
		json_path: &PathBuf,
		type_id: &String,
		asset: engine::asset::AnyBox,
		write_to: &PathBuf,
	) -> engine::utility::VoidResult {
		fs::create_dir_all(&write_to.parent().unwrap())?;
		let metadata = self.editor_metadata.get(type_id.as_str()).unwrap();
		let bytes = metadata.compile(&json_path, asset)?;
		fs::write(write_to, bytes)?;
		Ok(())
	}
}

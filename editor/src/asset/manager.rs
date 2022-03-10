use crate::asset;
use anyhow::Result;
use engine::{
	self,
	asset::{AnyBox, Generic, TypeId},
};
use serde_json;
use std::{
	collections::HashMap,
	fs,
	path::{Path, PathBuf},
	time::SystemTime,
};

type EditorMetadataBox = Box<dyn asset::TypeEditorMetadata>;

pub enum SupportedFileTypes {
	Json,
	Kdl,
}

impl SupportedFileTypes {
	pub fn all() -> Vec<Self> {
		vec![Self::Json, Self::Kdl]
	}

	pub fn extension(&self) -> &'static str {
		match self {
			Self::Json => "json",
			Self::Kdl => "kdl",
		}
	}

	pub fn parse_extension(ext: Option<&str>) -> Option<Self> {
		match ext {
			Some("json") => Some(Self::Json),
			Some("kdl") => Some(Self::Kdl),
			_ => None,
		}
	}
}

#[derive(Debug)]
pub enum KdlParsingError {
	MissingField(String),
}
impl std::error::Error for KdlParsingError {}
impl std::fmt::Display for KdlParsingError {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match self {
			Self::MissingField(ref message) => write!(f, "Missing field: {}", message),
		}
	}
}

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

	fn metadata<'r>(&self, type_id: &'r str) -> Result<&EditorMetadataBox> {
		let metadata = self.editor_metadata.get(type_id).ok_or(
			engine::asset::Error::UnregisteredAssetType(type_id.to_string()),
		)?;
		Ok(metadata)
	}

	fn read_type_id_sync(&self, path: &Path) -> Result<String> {
		let absolute_path = path.canonicalize()?;
		let raw_file = fs::read_to_string(&absolute_path)?;
		let type_id = Manager::read_asset_type(path.extension(), raw_file.as_str())?;
		Ok(type_id)
	}

	pub fn last_modified(&self, path: &Path) -> Result<SystemTime> {
		let type_id = self.read_type_id_sync(path)?;
		let metadata = self.metadata(&type_id)?;
		metadata.last_modified(&path)
	}

	/// Synchronously reads an asset json from a provided path, returning relevant asset loading errors.
	pub fn read_sync(&self, path: &Path) -> Result<(String, AnyBox)> {
		let absolute_path = path.canonicalize()?;
		let raw_file = fs::read_to_string(&absolute_path)?;
		let type_id = Manager::read_asset_type(path.extension(), raw_file.as_str())?;
		let asset = self
			.editor_metadata
			.get(type_id.as_str())
			.ok_or(engine::asset::Error::UnregisteredAssetType(
				type_id.to_string(),
			))?
			.read(&absolute_path, raw_file.as_str())?;
		Ok((type_id, asset))
	}

	fn read_asset_type(extension: Option<&std::ffi::OsStr>, content: &str) -> Result<String> {
		let ext = extension.map(|ext| ext.to_str()).flatten();
		match SupportedFileTypes::parse_extension(ext) {
			Some(SupportedFileTypes::Json) => {
				let generic: Generic = serde_json::from_str(content)?;
				return Ok(generic.asset_type);
			}
			Some(SupportedFileTypes::Kdl) => {
				let nodes = kdl::parse_document(&content)?;
				let asset_type_node = nodes.first().ok_or(KdlParsingError::MissingField(
					"No node named \"asset_type\".".to_owned(),
				))?;
				let asset_type_value =
					asset_type_node
						.values
						.first()
						.ok_or(KdlParsingError::MissingField(
							"\"asset_type\" has no value.".to_owned(),
						))?;
				Ok(match asset_type_value {
					kdl::KdlValue::String(asset_type) => Ok(asset_type.clone()),
					_ => Err(KdlParsingError::MissingField(
						"Value for \"asset_type\" is not a string.".to_owned(),
					)),
				}?)
			}
			None => Err(engine::asset::Error::ExtensionNotSupported(
				ext.map(|ext| ext.to_owned()),
			))?,
		}
	}

	pub fn compile(
		&self,
		json_path: &PathBuf,
		relative_path: &PathBuf,
		type_id: &String,
		mut asset: engine::asset::AnyBox,
		write_to: &PathBuf,
	) -> anyhow::Result<()> {
		fs::create_dir_all(&write_to.parent().unwrap())?;
		let metadata = self.editor_metadata.get(type_id.as_str()).unwrap();
		metadata.process_intermediate(&json_path, &relative_path, &mut asset)?;
		let bytes = metadata.compile(&json_path, asset)?;
		fs::write(write_to, bytes)?;
		Ok(())
	}
}

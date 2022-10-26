use crate::asset::{BuildPath, EditorOps};
use anyhow::Result;
use engine::{
	self,
	asset::{AnyBox, Generic, TypeId, UnregisteredAssetType},
	task::PinFutureResult,
};
use serde_json;
use std::{
	collections::HashMap,
	path::{Path, PathBuf},
	time::SystemTime,
};

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

pub struct Registration {
	fn_get_related_paths:
		Box<dyn Fn(PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> + 'static + Send + Sync>,
	fn_read: Box<dyn Fn(PathBuf, String) -> PinFutureResult<AnyBox> + 'static + Send + Sync>,
	fn_compile: Box<dyn Fn(BuildPath, AnyBox) -> PinFutureResult<Vec<u8>> + 'static + Send + Sync>,
}

impl Registration {
	fn new<T>() -> Self
	where
		T: EditorOps,
		<T as EditorOps>::Asset: engine::asset::Asset,
	{
		Self {
			fn_get_related_paths: Box::new(|path| T::get_related_paths(path)),
			fn_read: Box::new(|path, content| T::read(path, content)),
			fn_compile: Box::new(|build_path, asset| T::compile(build_path, asset)),
		}
	}

	pub fn get_related_paths(&self, path: PathBuf) -> PinFutureResult<Option<Vec<PathBuf>>> {
		(self.fn_get_related_paths)(path)
	}

	pub fn read(&self, source: PathBuf, file_content: String) -> PinFutureResult<AnyBox> {
		(self.fn_read)(source, file_content)
	}

	pub fn compile(&self, build_path: BuildPath, asset: AnyBox) -> PinFutureResult<Vec<u8>> {
		(self.fn_compile)(build_path, asset)
	}

	pub async fn last_modified_at(&self, path: &Path) -> anyhow::Result<SystemTime> {
		assert!(path.exists());
		let mut last_modified = path.metadata()?.modified()?;
		if let Some(paths) = self.get_related_paths(path.to_owned()).await? {
			for path in paths.into_iter() {
				if path.exists() {
					last_modified = last_modified.max(path.metadata()?.modified()?);
				}
			}
		}
		Ok(last_modified)
	}
}

/// Handles creating, saving, loading, moving, and deleting an asset at a given path.
/// Only accessible during editor-runtime whereas [Loader](engine::asset::Loader)
/// handles loading built assets during game-runtime.
pub struct Manager {
	registrations: HashMap<TypeId, Registration>,
}

impl Manager {
	pub fn new() -> Self {
		Self {
			registrations: HashMap::new(),
		}
	}

	pub fn register<T>(&mut self)
	where
		T: EditorOps,
		T::Asset: engine::asset::Asset,
	{
		use engine::asset::Asset;
		assert!(!self.registrations.contains_key(T::Asset::asset_type()));
		self.registrations
			.insert(T::Asset::asset_type(), Registration::new::<T>());
	}

	pub fn get(&self, type_id: &str) -> Result<&Registration, UnregisteredAssetType> {
		self.registrations
			.get(type_id)
			.ok_or(UnregisteredAssetType(type_id.to_string()))
	}

	pub async fn last_modified_at(&self, path: &Path) -> anyhow::Result<SystemTime> {
		let type_id = Self::read_type_id_sync(path).await?;
		let registration = self.get(&type_id)?;
		registration.last_modified_at(path).await
	}

	async fn read_type_id_sync(path: &Path) -> anyhow::Result<String> {
		let absolute_path = path.canonicalize()?;
		let raw_file = tokio::fs::read_to_string(&absolute_path).await?;
		let type_id = Self::parse_asset_type(path.extension(), raw_file.as_str())?;
		Ok(type_id)
	}

	pub async fn read(&self, path: &Path) -> Result<(String, AnyBox)> {
		let absolute_path = path.canonicalize()?;
		let raw_file = tokio::fs::read_to_string(&absolute_path).await?;
		let type_id = Self::parse_asset_type(path.extension(), raw_file.as_str())?;
		let registration = self.get(&type_id)?;
		let asset = registration.read(absolute_path, raw_file).await?;
		Ok((type_id, asset))
	}

	fn parse_asset_type(extension: Option<&std::ffi::OsStr>, content: &str) -> Result<String> {
		let ext = extension.map(|ext| ext.to_str()).flatten();
		match SupportedFileTypes::parse_extension(ext) {
			Some(SupportedFileTypes::Json) => {
				let generic: Generic = serde_json::from_str(content)?;
				return Ok(generic.asset_type);
			}
			Some(SupportedFileTypes::Kdl) => {
				let document = content.parse::<kdl::KdlDocument>()?;
				let asset_type_node =
					document
						.nodes()
						.first()
						.ok_or(KdlParsingError::MissingField(
							"No node named \"asset_type\".".to_owned(),
						))?;
				let asset_type_value =
					asset_type_node
						.entries()
						.first()
						.ok_or(KdlParsingError::MissingField(
							"\"asset_type\" has no value.".to_owned(),
						))?;
				Ok(match asset_type_value.value() {
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
}

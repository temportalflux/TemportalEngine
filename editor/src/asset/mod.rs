mod error;
pub use error::*;

mod manager;
pub use manager::*;

mod build;
pub use build::*;

mod metadata;
pub use metadata::*;

mod package;
pub use package::*;

pub fn deserialize<'a, T>(path: &std::path::Path, content: &'a str) -> engine::asset::AssetResult
where
	T: 'static + Send + Sync + serde::Deserialize<'a> + engine::asset::kdl::Asset<T> + Default,
{
	Ok(deserialize_typed::<T>(&path, &content).map(|t| Box::new(t))?)
}

pub fn deserialize_typed<'a, T>(path: &std::path::Path, content: &'a str) -> anyhow::Result<T>
where
	T: 'static + Send + Sync + serde::Deserialize<'a> + engine::asset::kdl::Asset<T> + Default,
{
	let ext = path.extension().map(|ext| ext.to_str()).flatten();
	match SupportedFileTypes::parse_extension(ext) {
		Some(SupportedFileTypes::Json) => Ok(serde_json::from_str::<T>(content)?),
		Some(SupportedFileTypes::Kdl) => Ok(T::kdl_schema().parse_and_validate(&content)?),
		_ => Err(engine::asset::Error::ExtensionNotSupported(
			ext.map(|ext| ext.to_owned()),
		))?,
	}
}

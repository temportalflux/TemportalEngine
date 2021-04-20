use crate::utility;

pub type AssetBox = Box<dyn Asset>;
pub type AssetResult = Result<AssetBox, utility::AnyError>;

pub type TypeId = &'static str;
pub trait TypeMetadata {
	fn name(&self) -> TypeId;
	fn read(&self, path: &std::path::Path, json_str: &str) -> AssetResult;
	fn compile(
		&self,
		json_path: &std::path::Path,
		asset: &AssetBox,
	) -> Result<Vec<u8>, utility::AnyError>;
}

pub trait Asset: std::fmt::Debug + crate::utility::AToAny {
	fn metadata() -> Box<dyn TypeMetadata>
	where
		Self: Sized;
}

pub fn as_asset<T>(asset: &AssetBox) -> &T
where
	T: Asset,
{
	asset.as_any().downcast_ref::<T>().unwrap()
}

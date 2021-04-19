pub type AssetResult = Result<Box<dyn Asset>, Box<dyn std::error::Error>>;

pub type TypeName = &'static str;
pub trait TypeMetadata {
	fn name(&self) -> TypeName;
	fn read(&self, path: &std::path::Path, json_str: &str) -> AssetResult;
}

pub trait Asset: std::fmt::Debug + crate::utility::AToAny {
	fn metadata() -> Box<dyn TypeMetadata>
	where
		Self: Sized;
}

pub fn as_asset<T>(asset: &Box<dyn Asset>) -> &T
where
	T: Asset,
{
	asset.as_any().downcast_ref::<T>().unwrap()
}

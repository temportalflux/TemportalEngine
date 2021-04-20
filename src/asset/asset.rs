use crate::utility;

pub type AssetBox = Box<dyn Asset>;
pub type AssetResult = Result<AssetBox, utility::AnyError>;

pub type TypeId = &'static str;
pub trait TypeMetadata {
	fn name(&self) -> TypeId;
	fn decompile(&self, bin: &Vec<u8>) -> AssetResult;
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

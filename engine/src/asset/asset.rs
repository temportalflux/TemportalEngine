use crate::utility;
use serde::{Deserialize, Serialize};
use std::any::Any;

pub type AnyBox = Box<dyn Any + Send + Sync>;
pub type AssetResult = Result<AnyBox, utility::AnyError>;

pub type TypeId = &'static str;
pub type TypeIdOwned = String;
pub trait TypeMetadata {
	fn name(&self) -> TypeId;
	fn decompile(&self, bin: &Vec<u8>) -> AssetResult;
}

pub trait Asset: std::fmt::Debug {
	fn metadata() -> Box<dyn TypeMetadata>
	where
		Self: Sized;
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Generic {
	pub asset_type: String,
}

pub fn decompile_asset<'a, T>(bin: &'a Vec<u8>) -> AssetResult
where
	T: Asset + Any + Send + Sync + Deserialize<'a>,
{
	Ok(Box::new(rmp_serde::from_read_ref::<'a, Vec<u8>, T>(&bin)?))
}

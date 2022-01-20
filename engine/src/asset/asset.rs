use crate::utility::Result;
use serde::{Deserialize, Serialize};
use std::any::Any;

pub type AnyBox = Box<dyn Any + Send + Sync>;
pub type AssetResult = Result<AnyBox>;

pub type TypeId = &'static str;
pub type TypeIdOwned = String;
pub trait TypeMetadata {
	fn name(&self) -> TypeId;
	fn decompile(&self, bin: &Vec<u8>) -> AssetResult;
	fn kdl_schema(&self) {}
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

pub mod kdl {
	pub trait Asset<T> {
		fn kdl_schema() -> kdl_schema::Schema<T>;
	}

	pub mod asset_type {
		use kdl_schema::*;

		pub fn schema<T>(on_validation_successful: fn(&mut T, &kdl::KdlNode)) -> Node<T> {
			Node {
				name: Name::Defined("asset-type"),
				values: Items::Ordered(vec![Value::String(None)]),
				on_validation_successful: Some(on_validation_successful),
				..Default::default()
			}
		}

		pub fn get(node: &kdl::KdlNode) -> String {
			utility::value_as_string(&node, 0).unwrap().clone()
		}
	}

	pub mod typed_enum {
		use kdl_schema::*;
		use std::convert::TryFrom;

		pub fn schema<TStruct, TEnum>(
			name: Name,
			on_validation_successful: fn(&mut TStruct, &kdl::KdlNode),
		) -> Node<TStruct>
		where
			TEnum: Into<String> + enumset::EnumSetType,
		{
			Node {
				name,
				values: Items::Ordered(vec![Value::String(Some(Validation::InList(
					enumset::EnumSet::all()
						.iter()
						.map(|value: TEnum| value.into())
						.collect(),
				)))]),
				on_validation_successful: Some(on_validation_successful),
				..Default::default()
			}
		}

		pub fn get<'a, TEnum>(node: &'a kdl::KdlNode) -> TEnum
		where
			TEnum: TryFrom<&'a str>,
		{
			let kind_string = utility::value_as_string(&node, 0).unwrap();
			match TEnum::try_from(kind_string.as_str()) {
				Ok(e) => e,
				Err(_err) => unimplemented!(),
			}
		}
	}
}

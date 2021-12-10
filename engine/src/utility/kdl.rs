use crate::asset;
use kdl_schema::utility::value_as_string;
use std::convert::TryFrom;

pub fn value_as_asset_id(node: &kdl::KdlNode, index: usize) -> Option<asset::Id> {
	value_as_string(node, index)
		.map(|s| asset::Id::try_from(s.as_str()).ok())
		.flatten()
}

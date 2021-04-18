use std::collections::HashMap;

pub type TypeName = &'static str;
pub struct TypeData {
	pub name: TypeName,
}

/// Runtime listings of the kinds of assets that exist.
/// Each asset type corresponds one-to-one with an implementation of the [`Asset`](crate::asset::Asset) trait.
pub struct TypeRegistry {
	types: HashMap<TypeName, TypeData>,
}

impl TypeRegistry {
	pub fn new() -> TypeRegistry {
		TypeRegistry {
			types: HashMap::new(),
		}
	}

	pub fn register(&mut self, type_data: TypeData) {
		self.types.insert(type_data.name, type_data);
	}
}

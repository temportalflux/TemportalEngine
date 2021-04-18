use crate::asset::TypeData;

pub trait Asset {
	fn type_data() -> TypeData;
}

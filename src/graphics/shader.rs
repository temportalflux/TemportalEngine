use crate::asset;

pub struct Shader {}

impl asset::Asset for Shader {
	fn type_data() -> asset::TypeData {
		asset::TypeData { name: "shader" }
	}
}

use crate::{
	asset,
	graphics::{self, flags, shader},
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

pub struct ShaderSet {
	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
}

impl Default for ShaderSet {
	fn default() -> Self {
		Self {
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
		}
	}
}

impl ShaderSet {
	#[profiling::function]
	pub fn insert(&mut self, id: &asset::Id) -> VoidResult {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();
		self.pending_shaders
			.insert(shader.kind(), shader.contents().clone());
		Ok(())
	}

	#[profiling::function]
	pub fn create_modules(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		for (kind, binary) in self.pending_shaders.drain() {
			self.shaders.insert(
				kind,
				sync::Arc::new(shader::Module::create(
					render_chain.logical().clone(),
					shader::Info {
						kind: kind,
						entry_point: String::from("main"),
						bytes: binary,
					},
				)?),
			);
		}
		Ok(())
	}
}

impl std::ops::Index<flags::ShaderKind> for ShaderSet {
	type Output = sync::Arc<shader::Module>;
	fn index(&self, kind: flags::ShaderKind) -> &Self::Output {
		self.shaders.get(&kind).unwrap()
	}
}

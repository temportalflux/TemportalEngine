//! Module for all rendering/graphics specific structures on the engine level.
//! Leverages/re-exports [`vulkan_rs`].
pub use vulkan_rs::*;

pub static LOG: &'static str = "graphics";

pub mod camera;

mod drawable;
pub use drawable::*;

pub mod debug {
	pub static LABEL_COLOR_RENDER_PASS: [f32; 4] = [0.098, 0.353, 0.0196, 1.0]; // #195a05
	pub static LABEL_COLOR_SUB_PASS: [f32; 4] = [0.063, 0.781, 0.333, 1.0]; // #10c755
	pub static LABEL_COLOR_DRAW: [f32; 4] = [0.298, 0.6, 0.875, 1.0]; // #4c99df
	pub static LABEL_COLOR_PRESENT: [f32; 4] = [0.34, 0.58, 0.267, 1.0]; // #579444
}

mod descriptor_cache;
pub use descriptor_cache::*;

pub mod font;

mod image_cache;
pub use image_cache::*;

mod mesh;
pub use mesh::*;

mod render_chain;
pub use render_chain::*;

pub mod render_pass;

#[path = "shader.rs"]
mod shader_engine;
pub use shader_engine::*;

mod shader_set;
pub use shader_set::*;

mod task;
pub use task::*;

mod texture;
pub use texture::*;

pub mod types;

mod uniform;
pub use uniform::*;

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Shader>();
	type_reg.register::<font::Font>();
	type_reg.register::<Texture>();
	render_pass::register_asset_types(type_reg);
}

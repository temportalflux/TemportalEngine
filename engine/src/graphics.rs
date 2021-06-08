//! Module for all rendering/graphics specific structures on the engine level.
//! Leverages/re-exports [`vulkan_rs`].
pub use vulkan_rs::*;

pub static LOG: &'static str = "graphics";

pub mod camera;

mod drawable;
pub use drawable::*;

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

#[path = "graphics/shader.rs"]
mod shader_engine;
pub use shader_engine::*;

mod shader_set;
pub use shader_set::*;

mod task;
pub use task::*;

mod texture;
pub use texture::*;

pub mod types;

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Shader>();
	type_reg.register::<font::Font>();
	type_reg.register::<Texture>();
	render_pass::register_asset_types(type_reg);
}

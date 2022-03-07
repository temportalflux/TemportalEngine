pub mod font;
pub mod render_pass;

mod shader;
pub use shader::*;

mod texture;
pub use texture::*;

mod procedure_config;
pub use procedure_config::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	use crate::engine::graphics::{font::Font, Shader, Texture};
	manager.register::<Shader, ShaderEditorMetadata>();
	manager.register::<Font, font::EditorMetadata>();
	manager.register::<Texture, TextureEditorMetadata>();
	render_pass::register_asset_types(manager);
}

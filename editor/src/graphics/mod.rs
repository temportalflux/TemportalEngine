pub mod font;

mod shader;
pub use shader::*;

mod texture;
pub use texture::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	use crate::engine::graphics::{font::Font, Shader, Texture};
	manager.register::<Shader, ShaderEditorMetadata>();
	manager.register::<Font, font::EditorMetadata>();
	manager.register::<Texture, TextureEditorMetadata>();
}

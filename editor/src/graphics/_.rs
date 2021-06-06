mod font;
pub use font::*;

pub mod render_pass;

mod shader;
pub use shader::*;

mod texture;
pub use texture::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	use crate::engine::graphics::{font, Shader, Texture};
	manager.register::<Shader, ShaderEditorMetadata>();
	manager.register::<font::Font, FontEditorMetadata>();
	manager.register::<Texture, TextureEditorMetadata>();
	render_pass::register_asset_types(manager);
}

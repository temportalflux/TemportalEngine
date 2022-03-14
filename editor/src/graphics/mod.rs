pub mod font;

mod shader;
pub use shader::*;

mod texture;
pub use texture::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	manager.register::<ShaderEditorOps>();
	manager.register::<font::FontEditorOps>();
	manager.register::<TextureEditorOps>();
}

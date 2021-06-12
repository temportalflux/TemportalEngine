mod sound;
pub use sound::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	use crate::engine::audio::Sound;
	manager.register::<Sound, SoundEditorMetadata>();
}

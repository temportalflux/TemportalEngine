mod pass;
pub use pass::*;
mod subpass;
pub use subpass::*;
mod attachment;
pub use attachment::*;

pub fn register_asset_types(manager: &mut crate::asset::Manager) {
	use crate::engine::graphics::render_pass::{Attachment, Pass, Subpass};
	manager.register::<Pass, PassEditorMetadata>();
	manager.register::<Subpass, SubpassEditorMetadata>();
	manager.register::<Attachment, AttachmentEditorMetadata>();
}

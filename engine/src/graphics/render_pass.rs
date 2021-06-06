mod pass;
pub use pass::*;
mod subpass;
pub use subpass::*;
mod attachment;
pub use attachment::*;

pub fn register_asset_types(type_reg: &mut crate::asset::TypeRegistry) {
	type_reg.register::<Pass>();
	type_reg.register::<Subpass>();
	type_reg.register::<Attachment>();
}

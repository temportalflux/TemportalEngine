extern crate bytemuck;
extern crate egui;
extern crate shaderc;

pub static LOG: &'static str = "editor";

pub use engine;

pub mod asset;
pub mod audio;

mod editor;
pub use editor::*;

pub mod graphics;

pub mod ui;

pub mod settings;

pub mod config;

pub fn register_asset_types(registry: &mut asset::Manager) {
	audio::register_asset_types(registry);
	graphics::register_asset_types(registry);
}

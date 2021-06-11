extern crate imgui;
extern crate shaderc;

pub static LOG: &'static str = "editor";

pub use temportal_engine as engine;

pub fn manifest_location() -> &'static str {
	std::env!("CARGO_MANIFEST_DIR")
}

#[path = "asset/_.rs"]
pub mod asset;

mod editor;
pub use editor::*;

#[path = "graphics/_.rs"]
pub mod graphics;

#[path = "ui/_.rs"]
pub mod ui;

pub mod settings;

pub mod config;

extern crate imgui;
extern crate shaderc;

pub static LOG: &'static str = "editor";

pub use temportal_engine as engine;

pub fn manifest_location() -> &'static str {
	std::env!("CARGO_MANIFEST_DIR")
}

pub mod asset;

mod editor;
pub use editor::*;

pub mod graphics;

pub mod ui;

pub mod settings;

pub mod config;

extern crate imgui;
extern crate shaderc;

pub static LOG: &'static str = "editor";

pub use temportal_engine as engine;

#[path = "asset/_.rs"]
pub mod asset;

mod editor;
pub use editor::*;

#[path = "graphics/_.rs"]
pub mod graphics;

#[path = "ui/_.rs"]
pub mod ui;

pub mod settings;

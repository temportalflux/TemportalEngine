extern crate bytemuck;
extern crate egui;
extern crate shaderc;

pub static LOG: &'static str = "editor";

pub use temportal_engine as engine;

pub mod asset;
pub mod audio;

mod editor;
pub use editor::*;

pub mod graphics;

pub mod ui;

pub mod settings;

pub mod config;

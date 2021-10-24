pub static LOG: &'static str = "ui";

pub mod core;
pub mod egui;
pub mod oui;
pub mod raui;

mod system;
pub use system::*;

pub static LOG: &'static str = "assets";

mod asset;
pub use asset::*;

mod error;
pub use error::*;

mod id;
pub use id::*;

mod library;
pub use library::*;

mod loader;
pub use loader::*;

mod types;
pub use types::*;

pub mod statics;

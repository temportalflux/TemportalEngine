pub use specs::{prelude::*, *};
pub mod components;
pub mod resources;
pub mod systems;

mod context;
pub use context::*;

pub trait NamedSystem: Sized {
	fn name() -> &'static str;
	fn dependencies(&self) -> Vec<&'static str> {
		vec![]
	}
}

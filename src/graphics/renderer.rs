use crate::{display, utility};
use temportal_graphics::command;

/// An object which contains data that needs to be updated when the render-chain is reconstructed
/// (i.e. something which contains a pipeline, and is therefore reliant on the resolution of the window).
pub trait RenderChainElement {
	fn initialize_with(&mut self, window: &display::Window) -> utility::Result<()>;
	fn on_render_chain_constructed(&mut self, window: &display::Window) -> utility::Result<()>;
}

/// An object which records commands to one or more command buffers,
/// notably when the render-chain is reconstructed.
pub trait CommandRecorder {
	fn record_to_buffer(&self, buffer: &mut command::Buffer) -> utility::Result<()>;
}

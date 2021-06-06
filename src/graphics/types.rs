//! Collection of attribute-safe types which conform to Vulkan specifications.
//!
//! Vulkan expects certain memory alignments in the data being sent,
//! where a struct of 2 sets of `[f32; 2]` may result in 1 `[f32; 4]`
//! being read by the GPU (instead of 2 attributes).
//!
//! In C/C++, this is solved by using `alignas(16)` on each field,
//! but that's both unavailable per-field in Rust and a lot to force
//! users to remember.
//!
//! The types provided in this module are safe to use in any
//! struct which implements [`vertex::Object`](super::pipeline::state::vertex::Object).
//!
//! Sources:
//! - [Memory Alignment article](https://fvcaputo.github.io/2019/02/06/memory-alignment.html)
//!
//! Example:
//! ```
//! #[vertex_object]
//! #[derive(Debug, Default)]
//! pub struct Vertex {
//!
//! 	#[vertex_attribute([R, G], Bit32, SFloat)]
//! 	pos: Vec2,
//!
//! 	#[vertex_attribute([R, G], Bit32, SFloat)]
//! 	tex_coord: Vec2,
//!
//! 	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
//! 	color: Vec4,
//!
//! }
//! ```
//! would appear in a shader as:
//! ```
//! layout(location = 0) in vec2 in_pos;
//! layout(location = 1) in vec2 in_tex_coord;
//! layout(location = 2) in vec4 in_color;
//! ```

/*
TODO: find some elegant way to support matrices...
*/

mod vec2;
pub use vec2::*;
mod vec3;
pub use vec3::*;
mod vec4;
pub use vec4::*;
mod mat4;
pub use mat4::*;

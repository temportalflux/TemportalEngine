use crate::graphics::{
	flags, pipeline,
	types::{Vec2, Vec4},
	vertex_object,
};

#[vertex_object]
#[derive(Debug, Default)]
pub struct Vertex {
	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	pub(crate) pos_and_width_edge: Vec4,

	#[vertex_attribute([R, G], Bit32, SFloat)]
	pub(crate) tex_coord: Vec2,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	pub(crate) color: Vec4,
}

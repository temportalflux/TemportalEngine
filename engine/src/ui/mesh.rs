use crate::{
	graphics::{
		self, flags, pipeline,
		types::{Vec2, Vec4},
		vertex_object,
	},
	math::nalgebra::Vector2,
};
use raui::renderer::tesselate::prelude::*;

pub type Mesh = graphics::Mesh<u32, Vertex>;

#[vertex_object]
#[derive(Debug, Default)]
pub struct Vertex {
	#[vertex_attribute([R, G], Bit32, SFloat)]
	pos: Vec2,

	#[vertex_attribute([R, G], Bit32, SFloat)]
	tex_coord: Vec2,

	#[vertex_attribute([R, G, B, A], Bit32, SFloat)]
	color: Vec4,
}

impl Vertex {
	pub(crate) fn create_interleaved_buffer_data(
		tesselation: &Tesselation,
		resolution: &Vector2<f32>,
	) -> (Vec<Vertex>, Vec<u32>) {
		(
			tesselation
				.vertices
				.as_interleaved()
				.unwrap()
				.into_iter()
				.map(|interleaved| Vertex::from_interleaved(interleaved, resolution))
				.collect::<Vec<_>>(),
			tesselation
				.indices
				.iter()
				.map(|i| *i as u32)
				.collect::<Vec<_>>(),
		)
	}

	fn from_interleaved(
		interleaved: &TesselationVerticeInterleaved,
		resolution: &Vector2<f32>,
	) -> Self {
		let TesselationVerticeInterleaved {
			position,
			tex_coord,
			color,
		} = interleaved;
		let pos: Vector2<f32> = [position.x as f32, position.y as f32].into();
		let pos = pos.component_div(resolution);
		let offset: Vector2<f32> = [-1.0, -1.0].into();
		let pos = pos * 2.0 + offset;
		Self {
			pos: [pos.x, pos.y].into(),
			tex_coord: [tex_coord.x, tex_coord.y].into(),
			color: [color.r, color.g, color.b, color.a].into(),
		}
	}
}

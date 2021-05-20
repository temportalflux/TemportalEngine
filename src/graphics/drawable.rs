use crate::{
	asset,
	graphics::{self, command, descriptor, flags, pipeline, ShaderSet},
	utility::{self, VoidResult},
};
use std::sync;

pub struct Drawable {
	pipeline: Option<pipeline::Pipeline>,
	pipeline_layout: Option<pipeline::Layout>,
	shaders: ShaderSet,
}

impl Default for Drawable {
	fn default() -> Self {
		Self {
			shaders: ShaderSet::default(),
			pipeline_layout: None,
			pipeline: None,
		}
	}
}

impl Drawable {
	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		self.shaders.insert(id)
	}

	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		self.shaders.create_modules(render_chain)
	}

	#[profiling::function]
	pub fn destroy_pipeline(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	#[profiling::function]
	pub fn create_pipeline(
		&mut self,
		render_chain: &graphics::RenderChain,
		descriptor_layout: Option<&sync::Arc<descriptor::SetLayout>>,
		pipeline_info: pipeline::Info,
	) -> utility::Result<()> {
		self.pipeline_layout = Some(
			match descriptor_layout {
				Some(layout) => pipeline::Layout::builder().with_descriptors(layout),
				None => pipeline::Layout::builder(),
			}
			.build(render_chain.logical().clone())?,
		);
		self.pipeline = Some(
			pipeline_info
				.add_shader(sync::Arc::downgrade(
					&self.shaders[flags::ShaderKind::Vertex],
				))
				.add_shader(sync::Arc::downgrade(
					&self.shaders[flags::ShaderKind::Fragment],
				))
				.create_object(
					render_chain.logical().clone(),
					&self.pipeline_layout.as_ref().unwrap(),
					&render_chain.render_pass(),
				)?,
		);
		Ok(())
	}

	pub fn bind_pipeline(&self, buffer: &mut command::Buffer) {
		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
	}

	pub fn bind_descriptors(
		&self,
		buffer: &mut command::Buffer,
		descriptor_sets: Vec<&descriptor::Set>,
	) {
		buffer.bind_descriptors(
			flags::PipelineBindPoint::GRAPHICS,
			self.pipeline_layout.as_ref().unwrap(),
			0,
			descriptor_sets,
		);
	}
}

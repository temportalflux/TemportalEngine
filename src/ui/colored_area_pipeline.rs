use crate::{
	asset,
	graphics::{self, command, flags, pipeline, shader, structs},
	ui,
	utility::{self, VoidResult},
};
use std::{collections::HashMap, sync};

pub struct ColoredAreaPipeline {
	pipeline: Option<pipeline::Pipeline>,
	pipeline_layout: Option<pipeline::Layout>,

	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
}

impl ColoredAreaPipeline {
	pub fn new() -> Self {
		Self {
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
			pipeline_layout: None,
			pipeline: None,
		}
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();
		self.pending_shaders
			.insert(shader.kind(), shader.contents().clone());
		Ok(())
	}

	#[profiling::function]
	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		for (kind, binary) in self.pending_shaders.drain() {
			self.shaders.insert(
				kind,
				sync::Arc::new(shader::Module::create(
					render_chain.logical().clone(),
					shader::Info {
						kind: kind,
						entry_point: String::from("main"),
						bytes: binary,
					},
				)?),
			);
		}
		Ok(())
	}

	#[profiling::function]
	pub fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	#[profiling::function]
	pub fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		self.pipeline_layout =
			Some(pipeline::Layout::builder().build(render_chain.logical().clone())?);
		self.pipeline = Some(
			pipeline::Info::default()
				.add_shader(sync::Arc::downgrade(
					self.shaders.get(&flags::ShaderKind::Vertex).unwrap(),
				))
				.add_shader(sync::Arc::downgrade(
					self.shaders.get(&flags::ShaderKind::Fragment).unwrap(),
				))
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<ui::mesh::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(
					pipeline::ViewportState::default()
						.add_viewport(graphics::utility::Viewport::default().set_size(resolution))
						.add_scissor(graphics::utility::Scissor::default().set_size(resolution)),
				)
				.set_rasterization_state(pipeline::RasterizationState::default())
				.set_color_blending(pipeline::ColorBlendState::default().add_attachment(
					pipeline::ColorBlendAttachment {
						color_flags: flags::ColorComponent::R
							| flags::ColorComponent::G | flags::ColorComponent::B
							| flags::ColorComponent::A,
						blend: Some(pipeline::Blend {
							color: SrcAlpha * New + (One - SrcAlpha) * Old,
							alpha: One * New + Zero * Old,
						}),
					},
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
}

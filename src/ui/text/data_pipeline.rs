use crate::{
	asset,
	graphics::{self, command, descriptor, flags, font::Font, pipeline, sampler, shader, structs},
	math::Vector,
	ui::{
		self,
		text::{self, font},
	},
	utility::{self, VoidResult},
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

struct FontData {
	loaded: font::Loaded,
	descriptor_set: sync::Weak<descriptor::Set>,
}

pub struct DataPipeline {
	pending_font_atlases: HashMap<font::Id, font::PendingAtlas>,
	fonts: HashMap<font::Id, FontData>,

	pipeline: Option<pipeline::Pipeline>,
	pipeline_layout: Option<pipeline::Layout>,
	descriptor_layout: sync::Arc<descriptor::SetLayout>,

	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
	sampler: sync::Arc<sampler::Sampler>,
}

impl DataPipeline {
	#[profiling::function]
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		let descriptor_layout = sync::Arc::new(
			descriptor::SetLayout::builder()
				.with_binding(
					0,
					flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					1,
					flags::ShaderKind::Fragment,
				)
				.build(&render_chain.logical())?,
		);
		Ok(Self {
			sampler: sync::Arc::new(
				graphics::sampler::Sampler::builder()
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
					.build(&render_chain.logical())?,
			),
			descriptor_layout,
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
			pipeline_layout: None,
			pipeline: None,
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		let shader = asset::Loader::load_sync(&id)?
			.downcast::<graphics::Shader>()
			.unwrap();
		self.pending_shaders
			.insert(shader.kind(), shader.contents().clone());
		Ok(())
	}

	pub fn add_pending(&mut self, id: String, font: Box<Font>) {
		self.pending_font_atlases.insert(id, font.into());
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
	pub fn create_pending_font_atlases(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let mut pending_gpu_signals = Vec::new();
		if !self.pending_font_atlases.is_empty() {
			for (id, pending) in self.pending_font_atlases.drain() {
				let (loaded, mut signals) = pending.load(render_chain)?;
				pending_gpu_signals.append(&mut signals);

				let descriptor_set = render_chain
					.persistent_descriptor_pool()
					.write()
					.unwrap()
					.allocate_descriptor_sets(&vec![self.descriptor_layout.clone()])?
					.pop()
					.unwrap();

				{
					use graphics::descriptor::*;
					SetUpdate::default()
						.with(UpdateOperation::Write(WriteOp {
							destination: UpdateOperationSet {
								set: descriptor_set.clone(),
								binding_index: 0,
								array_element: 0,
							},
							kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
							object: ObjectKind::Image(vec![ImageKind {
								sampler: self.sampler.clone(),
								view: loaded.view().clone(),
								layout: flags::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
							}]),
						}))
						.apply(&render_chain.logical());
				}

				self.fonts.insert(
					id,
					FontData {
						loaded,
						descriptor_set,
					},
				);
			}
		}
		Ok(pending_gpu_signals)
	}

	#[profiling::function]
	pub fn update_or_create(
		&self,
		render_chain: &graphics::RenderChain,
		resolution: &Vector<u32, 2>,
		text: BatchExternalText,
		widget: Option<text::WidgetData>,
	) -> utility::Result<(text::WidgetData, Vec<sync::Arc<command::Semaphore>>)> {
		let font_data = self
			.fonts
			.get(&text.font)
			.ok_or(ui::Error::InvalidFont(text.font.clone()))?;
		let mut widget = match widget {
			Some(widget) => widget,
			None => text::WidgetData::new(&text, render_chain)?,
		};
		let signals =
			widget.write_buffer_data(&text, &font_data.loaded, render_chain, resolution)?;
		Ok((widget, signals))
	}

	#[profiling::function]
	pub fn destroy_render_chain(&mut self, _: &graphics::RenderChain) -> utility::Result<()> {
		self.pipeline = None;
		self.pipeline_layout = None;
		Ok(())
	}

	pub fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()> {
		use flags::blend::{Constant::*, Factor::*, Source::*};
		self.pipeline_layout = Some(
			pipeline::Layout::builder()
				.with_descriptors(&self.descriptor_layout)
				.build(render_chain.logical().clone())?,
		);
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
						.with_object::<text::Vertex>(0, flags::VertexInputRate::VERTEX),
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

	#[profiling::function]
	pub fn record_to_buffer(
		&self,
		buffer: &mut command::Buffer,
		widget: &text::WidgetData,
	) -> utility::Result<()> {
		let font_data = self
			.fonts
			.get(widget.font_id())
			.ok_or(ui::Error::InvalidFont(widget.font_id().clone()))?;

		buffer.bind_pipeline(
			&self.pipeline.as_ref().unwrap(),
			flags::PipelineBindPoint::GRAPHICS,
		);
		buffer.bind_descriptors(
			flags::PipelineBindPoint::GRAPHICS,
			self.pipeline_layout.as_ref().unwrap(),
			0,
			vec![&font_data.descriptor_set.upgrade().unwrap()],
		);
		widget.bind_buffers(buffer);
		buffer.draw(*widget.index_count(), 0, 1, 0, 0);
		Ok(())
	}

}

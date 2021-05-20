use crate::{
	asset,
	graphics::{
		self, command, descriptor, flags, font::Font, pipeline, sampler, structs, Drawable,
	},
	math::Vector,
	ui::{
		self,
		text::{self, font},
	},
	utility::{self, VoidResult},
};
use raui::renderer::tesselate::prelude::*;
use std::{collections::HashMap, sync};

struct FontData {
	loaded: font::Loaded,
	descriptor_set: sync::Weak<descriptor::Set>,
}

pub struct DataPipeline {
	pending_font_atlases: HashMap<font::Id, font::PendingAtlas>,
	fonts: HashMap<font::Id, FontData>,

	drawable: Drawable,
	descriptor_layout: sync::Arc<descriptor::SetLayout>,
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
			drawable: Drawable::default(),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> VoidResult {
		self.drawable.add_shader(id)
	}

	pub fn add_pending(
		&mut self,
		id: String,
		font: Box<Font>,
		get_width_edge: font::BoxedGetWidthEdge,
	) {
		self.pending_font_atlases.insert(
			id,
			font::PendingAtlas::from(font).with_width_edge(get_width_edge),
		);
	}

	#[profiling::function]
	pub fn create_shaders(&mut self, render_chain: &graphics::RenderChain) -> utility::Result<()> {
		self.drawable.create_shaders(render_chain)
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
	pub fn destroy_render_chain(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<()> {
		self.drawable.destroy_pipeline(render_chain)
	}

	pub fn on_render_chain_constructed(
		&mut self,
		render_chain: &graphics::RenderChain,
		resolution: structs::Extent2D,
	) -> utility::Result<()> {
		self.drawable.create_pipeline(
			render_chain,
			Some(&self.descriptor_layout),
			pipeline::Info::default()
				.with_vertex_layout(
					pipeline::vertex::Layout::default()
						.with_object::<text::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(pipeline::ViewportState::from(resolution))
				.set_rasterization_state(pipeline::RasterizationState::default())
				.set_color_blending(
					pipeline::ColorBlendState::default()
						.add_attachment(pipeline::ColorBlendAttachment::default()),
				),
		)
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

		self.drawable.bind_pipeline(buffer);
		self.drawable
			.bind_descriptors(buffer, vec![&font_data.descriptor_set.upgrade().unwrap()]);
		widget.bind_buffers(buffer);
		buffer.draw(*widget.index_count(), 0, 1, 0, 0);
		Ok(())
	}
}
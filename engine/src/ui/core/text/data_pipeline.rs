use crate::channels::mpsc::Sender;
use crate::{
	asset,
	graphics::{
		self, command,
		descriptor::{self, layout::SetLayout},
		device::logical,
		flags,
		font::Font,
		pipeline, sampler,
		utility::{BuildFromDevice, NameableBuilder},
		Chain, Drawable, GpuOpContext,
	},
	ui::{
		self,
		core::text::{self, font},
	},
};
use anyhow::Result;
use raui::{core::widget::WidgetId, renderer::tesselate::prelude::*};
use std::{
	collections::HashMap,
	sync::{self, Arc, RwLock},
};

struct FontData {
	loaded: font::Loaded,
	descriptor_set: sync::Weak<descriptor::Set>,
}

pub struct DataPipeline {
	pending_font_atlases: HashMap<font::Id, font::PendingAtlas>,
	fonts: HashMap<font::Id, FontData>,

	drawable: Drawable,
	descriptor_layout: sync::Arc<SetLayout>,
	sampler: sync::Arc<sampler::Sampler>,
}

impl DataPipeline {
	#[profiling::function]
	pub fn new(context: &impl GpuOpContext) -> anyhow::Result<Self> {
		let descriptor_layout = sync::Arc::new(
			SetLayout::builder()
				.with_name("UI.Text.DescriptorLayout")
				.with_binding(
					0,
					flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
					1,
					flags::ShaderKind::Fragment,
				)
				.build(&context.logical_device()?)?,
		);
		Ok(Self {
			sampler: sync::Arc::new(
				graphics::sampler::Sampler::builder()
					.with_name("UI.Text.Font.Sampler")
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(context.physical_device()?.max_sampler_anisotropy()))
					.build(&context.logical_device()?)?,
			),
			descriptor_layout,
			drawable: Drawable::default().with_name("UI.Text"),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
		})
	}

	pub fn add_shader(&mut self, id: &asset::Id) -> Result<()> {
		self.drawable.add_shader(id)
	}

	pub fn add_pending(&mut self, id: String, font: Box<Font>) {
		self.pending_font_atlases
			.insert(id.clone(), font::PendingAtlas::from(id, font));
	}

	#[profiling::function]
	pub fn create_shaders(&mut self, logical: &Arc<logical::Device>) -> anyhow::Result<()> {
		self.drawable.create_shaders(logical)
	}

	#[profiling::function]
	pub fn create_pending_font_atlases(
		&mut self,
		context: &impl GpuOpContext,
		descriptor_pool: &Arc<RwLock<descriptor::Pool>>,
		signal_sender: &Sender<Arc<command::Semaphore>>,
	) -> anyhow::Result<()> {
		if !self.pending_font_atlases.is_empty() {
			for (id, pending) in self.pending_font_atlases.drain() {
				let loaded = pending.load(context, signal_sender)?;

				let descriptor_set = descriptor_pool
					.write()
					.unwrap()
					.allocate_named_descriptor_sets(&vec![(
						self.descriptor_layout.clone(),
						format!("{}.Descriptor", loaded.name()),
					)])?
					.pop()
					.unwrap();

				{
					use graphics::descriptor::update::*;
					Queue::default()
						.with(Operation::Write(WriteOp {
							destination: Descriptor {
								set: descriptor_set.clone(),
								binding_index: 0,
								array_element: 0,
							},
							kind: graphics::flags::DescriptorKind::COMBINED_IMAGE_SAMPLER,
							object: ObjectKind::Image(vec![ImageKind {
								sampler: self.sampler.clone(),
								view: loaded.view().clone(),
								layout: flags::ImageLayout::ShaderReadOnlyOptimal,
							}]),
						}))
						.apply(&*context.logical_device()?);
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
		Ok(())
	}

	#[profiling::function]
	pub fn update_or_create(
		&self,
		chain: &Chain,
		widget_id: &WidgetId,
		text: BatchExternalText,
		widget: Option<text::WidgetData>,
	) -> anyhow::Result<text::WidgetData> {
		let font_data = self
			.fonts
			.get(&text.font)
			.ok_or(ui::core::Error::InvalidFont(text.font.clone()))?;
		let mut widget = match widget {
			Some(widget) => widget,
			None => text::WidgetData::new((*widget_id).to_string(), &text, &chain.allocator()?)?,
		};
		widget.write_buffer_data(
			&text,
			&font_data.loaded,
			chain,
			&chain.resolution(),
			chain.signal_sender(),
		)?;
		Ok(widget)
	}

	#[profiling::function]
	pub fn destroy_render_chain(&mut self) -> anyhow::Result<()> {
		self.drawable.destroy_pipeline()
	}

	pub fn create_pipeline(&mut self, chain: &Chain, subpass_index: usize) -> anyhow::Result<()> {
		use pipeline::state::*;
		self.drawable.create_pipeline(
			&chain.logical()?,
			vec![&self.descriptor_layout],
			pipeline::Pipeline::builder()
				.with_vertex_layout(
					vertex::Layout::default()
						.with_object::<text::Vertex>(0, flags::VertexInputRate::VERTEX),
				)
				.set_viewport_state(Viewport::from(*chain.extent()))
				.set_rasterization_state(Rasterization::default())
				.set_color_blending(
					color_blend::ColorBlend::default()
						.add_attachment(color_blend::Attachment::default()),
				)
				.with_dynamic_state(flags::DynamicState::SCISSOR),
			chain.render_pass(),
			subpass_index,
		)
	}

	#[profiling::function]
	pub fn record_to_buffer(
		&self,
		buffer: &mut command::Buffer,
		widget: &text::WidgetData,
	) -> anyhow::Result<()> {
		let font_data = self
			.fonts
			.get(widget.font_id())
			.ok_or(ui::core::Error::InvalidFont(widget.font_id().clone()))?;

		self.drawable.bind_pipeline(buffer);
		self.drawable
			.bind_descriptors(buffer, vec![&font_data.descriptor_set.upgrade().unwrap()]);
		widget.bind_buffers(buffer);
		buffer.draw(*widget.index_count(), 0, 1, 0, 0);
		Ok(())
	}
}

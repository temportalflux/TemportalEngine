use crate::{
	asset,
	graphics::{self, command, flags, font::Font, sampler, shader},
	math::Vector,
	ui::{
		self,
		text::{self, font},
	},
	utility::{self, VoidResult},
};
pub use raui::prelude::*;
use std::{collections::HashMap, sync};

pub struct DataPipeline {
	pending_font_atlases: HashMap<font::Id, font::PendingAtlas>,
	fonts: HashMap<font::Id, font::Loaded>,

	shaders: HashMap<flags::ShaderKind, sync::Arc<shader::Module>>,
	pending_shaders: HashMap<flags::ShaderKind, Vec<u8>>,
	sampler: sync::Arc<sampler::Sampler>,
}

impl DataPipeline {
	pub fn new(render_chain: &graphics::RenderChain) -> utility::Result<Self> {
		Ok(Self {
			sampler: sync::Arc::new(
				graphics::sampler::Sampler::builder()
					.with_address_modes([flags::SamplerAddressMode::REPEAT; 3])
					.with_max_anisotropy(Some(render_chain.physical().max_sampler_anisotropy()))
					.build(&render_chain.logical())?,
			),
			pending_shaders: HashMap::new(),
			shaders: HashMap::new(),
			fonts: HashMap::new(),
			pending_font_atlases: HashMap::new(),
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

	pub fn create_pending_font_atlases(
		&mut self,
		render_chain: &graphics::RenderChain,
	) -> utility::Result<Vec<sync::Arc<command::Semaphore>>> {
		let mut pending_gpu_signals = Vec::new();
		if !self.pending_font_atlases.is_empty() {
			for (id, pending) in self.pending_font_atlases.drain() {
				let (loaded, mut signals) = pending.load(render_chain)?;
				pending_gpu_signals.append(&mut signals);
				self.fonts.insert(id, loaded);
			}
		}
		Ok(pending_gpu_signals)
	}

	pub fn update_or_create(
		&self,
		render_chain: &graphics::RenderChain,
		resolution: &Vector<u32, 2>,
		text: BatchExternalText,
		widget: Option<text::WidgetData>,
	) -> utility::Result<(text::WidgetData, Vec<sync::Arc<command::Semaphore>>)> {
		let font = self
			.fonts
			.get(&text.font)
			.ok_or(ui::Error::InvalidFont(text.font.clone()))?;
		let mut widget = match widget {
			Some(widget) => widget,
			None => text::WidgetData::new(&text, render_chain)?,
		};
		let signals = widget.write_buffer_data(&text, font, render_chain, resolution)?;
		Ok((widget, signals))
	}
}

use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::{flags, renderpass::Info as GraphicsPassInfo},
	utility::AnyError,
};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Pass).
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Pass {
	asset_type: String,
	/// Defines the order that subpasses will be recorded in the [`RenderChain`](crate::graphics::RenderChain).
	/// This is not the order that the GPU will process the commands - that is defined by [`dependencies`](Pass::dependencies).
	subpass_order: Vec<asset::Id>,
	/// Defines what subpasses are required by other subpasses (and which is first) so the GPU
	/// knows how to composite the subpasses together.
	dependencies: Vec<Dependency>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct Dependency {
	first: DependencyItem,
	then: DependencyItem,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct DependencyItem {
	subpass: Option<asset::Id>,
	#[serde(default)]
	stage: Vec<flags::PipelineStage>,
	#[serde(default)]
	access: Vec<flags::Access>,
}

impl asset::Asset for Pass {
	fn metadata() -> Box<dyn TypeMetadata> {
		Box::new(PassMetadata {})
	}
}

/// The metadata about the [`Pass`] asset type.
pub struct PassMetadata;
impl TypeMetadata for PassMetadata {
	fn name(&self) -> asset::TypeId {
		"render-pass"
	}

	fn decompile(&self, bin: &Vec<u8>) -> AssetResult {
		asset::decompile_asset::<Pass>(bin)
	}
}

impl Pass {
	pub fn as_graphics(&self) -> Result<GraphicsPassInfo, AnyError> {
		use crate::{
			asset::Loader,
			graphics::{render_pass, renderpass},
		};
		let mut rp_info = GraphicsPassInfo::empty();

		let mut attachment_ids = HashSet::new();
		for id in self.subpass_order.iter() {
			let subpass = Loader::load_sync(id)?
				.downcast::<render_pass::Subpass>()
				.unwrap();
			let mut graphics = renderpass::Subpass::new(id.as_string());

			for attachment in subpass.attachments.input.iter() {
				attachment_ids.insert(attachment.id.clone());
				graphics =
					graphics.add_input_attachment(attachment.id.as_string(), attachment.layout);
			}
			for attachment in subpass.attachments.color.iter() {
				attachment_ids.insert(attachment.id.clone());
				graphics =
					graphics.add_color_attachment(attachment.id.as_string(), attachment.layout);
			}
			if let Some(attachment) = &subpass.attachments.depth_stencil {
				attachment_ids.insert(attachment.id.clone());
				graphics = graphics
					.with_depth_stencil_attachment(attachment.id.as_string(), attachment.layout);
			}

			rp_info.add_subpass(graphics);
		}

		for id in attachment_ids.iter() {
			let asset = Loader::load_sync(id)?;
			let attachment = asset.downcast::<render_pass::Attachment>().unwrap();
			rp_info.attach(
				renderpass::Attachment::new(id.as_string())
					.with_format(attachment.format().as_format())
					.with_sample_count(*attachment.sample_count())
					.with_general_ops(*attachment.general_ops())
					.with_stencil_ops(*attachment.stencil_ops())
					.with_initial_layout(*attachment.initial_layout())
					.with_final_layout(*attachment.final_layout()),
			);
		}

		for dependency_link in self.dependencies.iter() {
			rp_info.add_dependency(
				renderpass::Dependency::new(
					dependency_link
						.first
						.subpass
						.as_ref()
						.map(|id| id.as_string()),
				)
				.with_stage_set(flags::PipelineStage::vecset(&dependency_link.first.stage))
				.with_access_set(flags::Access::vecset(&dependency_link.first.access)),
				renderpass::Dependency::new(
					dependency_link
						.then
						.subpass
						.as_ref()
						.map(|id| id.as_string()),
				)
				.with_stage_set(flags::PipelineStage::vecset(&dependency_link.then.stage))
				.with_access_set(flags::Access::vecset(&dependency_link.then.access)),
			);
		}

		Ok(rp_info)
	}
}

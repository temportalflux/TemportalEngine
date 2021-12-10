use crate::{
	asset::{self, AssetResult, TypeMetadata},
	graphics::{flags, renderpass::Info as GraphicsPassInfo},
	utility::AnyError,
};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

/// The engine asset representation of a [`render pass`](crate::graphics::renderpass::Pass).
#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct Pass {
	asset_type: String,
	/// Defines the order that subpasses will be recorded in the [`RenderChain`](crate::graphics::RenderChain).
	/// This is not the order that the GPU will process the commands - that is defined by [`dependencies`](Pass::dependencies).
	subpass_order: Vec<asset::Id>,
	/// Defines what subpasses are required by other subpasses (and which is first) so the GPU
	/// knows how to composite the subpasses together.
	dependencies: Vec<Dependency>,
	#[serde(skip)]
	kdl_subpass_aliases: std::collections::HashMap<String, asset::Id>,
	#[serde(skip)]
	kdl_subpass_dependencies: Vec<(Option<String>, Option<String>, Dependency)>,
}

impl Pass {
	fn init_asset_type(&mut self, node: &kdl::KdlNode) {
		self.asset_type = asset::kdl::asset_type::get(node);
	}
	fn insert_subpass_mapping(&mut self, node: &kdl::KdlNode) {
		use std::convert::TryFrom;
		let asset_str = match &node.values[0] {
			kdl::KdlValue::String(asset_str) => asset_str.as_str(),
			_ => unimplemented!(),
		};
		let subpass_id = asset::Id::try_from(asset_str).unwrap();
		self.kdl_subpass_aliases
			.insert(node.name.clone(), subpass_id.clone());
		self.subpass_order.push(subpass_id);
	}
	fn insert_dependency(&mut self, node: &kdl::KdlNode) {
		let (first_name, first) = self.create_dependency_item(&node.children[0]);
		let (then_name, then) = self.create_dependency_item(&node.children[1]);
		self.kdl_subpass_dependencies
			.push((first_name, then_name, Dependency { first, then }));
	}
	fn create_dependency_item(&self, node: &kdl::KdlNode) -> (Option<String>, DependencyItem) {
		use std::convert::TryFrom;
		let mut subpass = None;
		let mut item = DependencyItem {
			subpass: None,
			stage: vec![],
			access: vec![],
		};
		if !node.values.is_empty() {
			let subpass_alias = match &node.values[0] {
				kdl::KdlValue::String(asset_str) => asset_str.as_str(),
				_ => unimplemented!(),
			};
			subpass = Some(subpass_alias.to_owned());
		}
		for (name, value) in node.properties.iter() {
			match name.as_str() {
				"stage" => {
					let flag = flags::PipelineStage::try_from(match &value {
						kdl::KdlValue::String(asset_str) => asset_str.as_str(),
						_ => unimplemented!(),
					})
					.unwrap();
					item.stage.push(flag);
				}
				"access" => {
					let flag = flags::Access::try_from(match &value {
						kdl::KdlValue::String(asset_str) => asset_str.as_str(),
						_ => unimplemented!(),
					})
					.unwrap();
					item.access.push(flag);
				}
				_ => {}
			}
		}
		(subpass, item)
	}

	fn finalize_kdl_deserialization(&mut self) {
		self.dependencies.clear();
		let kdl_deps = self.kdl_subpass_dependencies.drain(..).collect::<Vec<_>>();
		for (first_name, then_name, mut dependency) in kdl_deps.into_iter() {
			dependency.first.subpass = first_name
				.map(|alias| self.kdl_subpass_aliases.get(&alias))
				.flatten()
				.map(|id| id.clone());
			dependency.then.subpass = then_name
				.map(|alias| self.kdl_subpass_aliases.get(&alias))
				.flatten()
				.map(|id| id.clone());
			self.dependencies.push(dependency);
		}
		self.kdl_subpass_aliases.clear();
		self.kdl_subpass_dependencies.clear();
	}
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

impl crate::asset::kdl::Asset<Pass> for Pass {
	fn kdl_schema() -> kdl_schema::Schema<Pass> {
		use kdl_schema::*;
		Schema {
			nodes: Items::Ordered(vec![
				asset::kdl::asset_type::schema::<Pass>(Pass::init_asset_type),
				Node {
					name: Name::Defined("asset-type"),
					values: Items::Ordered(vec![Value::String(None)]),
					on_validation_successful: Some(Pass::init_asset_type),
					..Default::default()
				},
				/*
				subpasses {
					subpass1name "asset-module:path/to/subpass1_asset"
					subpass2name "asset-module:path/to/subpass2_asset"
					...
				}
				*/
				Node {
					name: Name::Defined("subpasses"),
					children: Items::Select(vec![Node {
						name: Name::Variable("subpasses"),
						values: Items::Ordered(vec![Value::String(None)]),
						on_validation_successful: Some(Pass::insert_subpass_mapping),
						..Default::default()
					}]),
					..Default::default()
				},
				/*
				dependencies {
					link {
						first stage="ColorAttachmentOutput"
						then "subpass1name" stage="ColorAttachmentOutput" access="ColorAttachmentWrite"
					}
					link {
						first "subpass1name" stage="ColorAttachmentOutput" access="ColorAttachmentWrite"
						then "subpass2name" stage="ColorAttachmentOutput" access="ColorAttachmentWrite"
					}
				}
				*/
				Node {
					name: Name::Defined("dependencies"),
					children: Items::Select(vec![Node {
						name: Name::Defined("link"),
						children: Items::Ordered(vec![
							Node {
								name: Name::Defined("first"),
								values: Items::Select(vec![
									Value::Null,
									Value::String(Some(Validation::IsInVariable("subpasses"))),
								]),
								properties: vec![
									Property {
										name: "stage",
										value: Value::String(Some(Validation::InList(
											flags::PipelineStage::all_serialized(),
										))),
										optional: false,
									},
									Property {
										name: "access",
										value: Value::String(Some(Validation::InList(
											flags::Access::all_serialized(),
										))),
										optional: true,
									},
								],
								..Default::default()
							},
							Node {
								name: Name::Defined("then"),
								values: Items::Select(vec![
									Value::Null,
									Value::String(Some(Validation::IsInVariable("subpasses"))),
								]),
								properties: vec![
									Property {
										name: "stage",
										value: Value::String(Some(Validation::InList(
											flags::PipelineStage::all_serialized(),
										))),
										optional: false,
									},
									Property {
										name: "access",
										value: Value::String(Some(Validation::InList(
											flags::Access::all_serialized(),
										))),
										optional: true,
									},
								],
								..Default::default()
							},
						]),
						on_validation_successful: Some(Pass::insert_dependency),
						..Default::default()
					}]),
					..Default::default()
				},
			]),
			on_validation_successful: Some(Pass::finalize_kdl_deserialization),
		}
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

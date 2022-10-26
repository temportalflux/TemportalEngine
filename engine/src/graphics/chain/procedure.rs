use crate::graphics::{resource::Registry, Chain};
use std::sync::Arc;
use vulkan_rs::{
	flags::{Access, AttachmentKind, AttachmentOps, ImageLayout, LoadOp, PipelineStage, StoreOp},
	procedure::{attachment, Attachment, Dependency, Phase, PhaseAccess, Procedure},
	renderpass::ClearValue,
};

pub trait ProcedureConfig {
	type Attachments: AttachmentConfig;
	type Phases: PhaseConfig<Self::Attachments>;
	type Resources: ResourceConfig<Self::Attachments>;

	fn apply_to(chain: &mut Chain) -> anyhow::Result<Self::Phases> {
		let attachments = Self::Attachments::new(chain)?;
		let phases = Self::Phases::new(&attachments)?;
		chain.set_procedure(
			{
				use crate::graphics::utility::NameableBuilder;
				let mut procedure = Procedure::default().with_name("RenderChain");
				phases.apply_to(&mut procedure)?;
				procedure
			},
			attachments.swapchain_attachment().clone(),
		);
		Self::Resources::create_resources(attachments, chain.resources_mut())?;
		Ok(phases)
	}
}

pub trait AttachmentConfig {
	fn new(chain: &Chain) -> anyhow::Result<Self>
	where
		Self: Sized;
	fn swapchain_attachment(&self) -> &Arc<Attachment>;
}

pub trait PhaseConfig<T> {
	fn new(attachments: &T) -> anyhow::Result<Self>
	where
		Self: Sized;
	fn apply_to(&self, procedure: &mut Procedure) -> anyhow::Result<()>;
}

pub trait ResourceConfig<T> {
	fn create_resources(attachments: T, resources: &mut Registry) -> anyhow::Result<()>;
}

pub struct DefaultProcedure;
impl ProcedureConfig for DefaultProcedure {
	type Attachments = SingleAttachment;
	type Phases = SinglePhase;
	type Resources = NoResources;
}

pub struct SingleAttachment(Arc<Attachment>);
impl AttachmentConfig for SingleAttachment {
	fn new(chain: &Chain) -> anyhow::Result<Self> {
		Ok(Self(Arc::new(
			Attachment::default()
				.with_format(chain.swapchain_image_format())
				.with_general_ops(AttachmentOps {
					load: LoadOp::Clear,
					store: StoreOp::Store,
				})
				.with_final_layout(ImageLayout::PresentSrc)
				.with_clear_value(ClearValue::Color([0.0, 0.0, 0.0, 1.0])),
		)))
	}

	fn swapchain_attachment(&self) -> &Arc<Attachment> {
		&self.0
	}
}

pub struct SinglePhase(Arc<Phase>);
impl SinglePhase {
	pub fn into_inner(self) -> Arc<Phase> {
		self.0
	}
}
impl<T> PhaseConfig<T> for SinglePhase
where
	T: AttachmentConfig,
{
	fn new(attachments: &T) -> anyhow::Result<Self> {
		Ok(Self(Arc::new(
			Phase::new("Render")
				.with_dependency(
					Dependency::new(None)
						.first(
							PhaseAccess::default().with_stage(PipelineStage::ColorAttachmentOutput),
						)
						.then(
							PhaseAccess::default()
								.with_stage(PipelineStage::ColorAttachmentOutput)
								.with_access(Access::ColorAttachmentWrite),
						),
				)
				.with_attachment(
					attachment::Reference::from(attachments.swapchain_attachment())
						.with_kind(AttachmentKind::Color)
						.with_layout(ImageLayout::ColorAttachmentOptimal),
				),
		)))
	}

	fn apply_to(&self, procedure: &mut Procedure) -> anyhow::Result<()> {
		procedure.add_phase(self.0.clone())?;
		Ok(())
	}
}

pub struct NoResources;
impl<T> ResourceConfig<T> for NoResources
where
	T: AttachmentConfig,
{
	fn create_resources(_attachments: T, _resources: &mut Registry) -> anyhow::Result<()> {
		Ok(())
	}
}

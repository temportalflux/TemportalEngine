use engine::graphics::{
	flags::{Access, AttachmentKind, AttachmentOps, ImageLayout, LoadOp, PipelineStage, StoreOp},
	procedure::*,
	renderpass::ClearValue,
	Chain,
};
use std::sync::{Arc, RwLock};

pub struct ProcedureConfig {
	frame: Arc<Attachment>,
	phase: Arc<Phase>,
}

impl ProcedureConfig {
	pub fn initialize_chain(chain: &Arc<RwLock<Chain>>) -> anyhow::Result<Arc<Phase>> {
		let mut chain = chain.write().unwrap();
		Self::new(&chain)?.apply_to(&mut chain)
	}

	fn new(chain: &Chain) -> anyhow::Result<Self> {
		let viewport_format = chain.swapchain_image_format();
		let frame = Arc::new(
			Attachment::default()
				.with_format(viewport_format)
				.with_general_ops(AttachmentOps {
					load: LoadOp::Clear,
					store: StoreOp::Store,
				})
				.with_final_layout(ImageLayout::PresentSrc)
				.with_clear_value(ClearValue::Color([0.0, 0.0, 0.0, 1.0])),
		);

		let phase = Arc::new(
			Phase::new("EGui")
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
					attachment::Reference::from(&frame)
						.with_kind(AttachmentKind::Color)
						.with_layout(ImageLayout::ColorAttachmentOptimal),
				),
		);

		Ok(Self { frame, phase })
	}

	fn apply_to(self, chain: &mut Chain) -> anyhow::Result<Arc<Phase>> {
		let procedure = Procedure::default().with_phase(self.phase.clone())?;
		chain.set_procedure(procedure, self.frame);
		Ok(self.phase)
	}
}

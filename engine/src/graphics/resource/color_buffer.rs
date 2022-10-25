use super::{Id, Resource};
use crate::graphics::{
	command::frame::AttachedView,
	flags::SampleCount,
	flags::{self},
	image::Image,
	image_view::View,
	procedure::Attachment,
	structs,
	utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder},
	Chain,
};
use std::sync::{Arc, Weak};

#[derive(Default)]
pub struct ColorBufferBuilder {
	attachment: Option<Arc<Attachment>>,
}

impl ColorBufferBuilder {
	pub fn with_attachment(mut self, attachment: Arc<Attachment>) -> Self {
		self.attachment = Some(attachment);
		self
	}

	pub fn build(self) -> ColorBuffer {
		ColorBuffer {
			attachment: self.attachment.unwrap(),
			view: None,
		}
	}
}

pub struct ColorBuffer {
	attachment: Arc<Attachment>,
	view: Option<Arc<View>>,
}

impl ColorBuffer {
	pub fn builder() -> ColorBufferBuilder {
		ColorBufferBuilder::default()
	}

	pub fn sample_count(&self) -> SampleCount {
		self.attachment.sample_count()
	}
}

impl Resource for ColorBuffer {
	fn unique_id() -> Id {
		"ColorBuffer"
	}

	fn construct(&mut self, chain: &Chain) -> anyhow::Result<()> {
		let extent = chain.extent();

		let image = Arc::new(
			Image::builder()
				.with_name(format!("{}.Image", Self::unique_id()))
				.with_location(flags::MemoryLocation::GpuOnly)
				.with_format(self.attachment.format())
				.with_sample_count(self.sample_count())
				.with_usage(flags::ImageUsage::COLOR_ATTACHMENT)
				.with_usage(flags::ImageUsage::TRANSIENT_ATTACHMENT)
				.with_size(structs::Extent3D {
					width: extent.width,
					height: extent.height,
					depth: 1,
				})
				.build(&chain.allocator()?)?,
		);

		self.view = Some(Arc::new(
			View::builder()
				.with_name(format!("{}.View", Self::unique_id()))
				.for_image(image)
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(
					structs::subresource::Range::default().with_aspect(flags::ImageAspect::COLOR),
				)
				.build(&chain.logical()?)?,
		));

		Ok(())
	}

	fn get_attachments(&self) -> Vec<(Weak<Attachment>, AttachedView)> {
		let view = self.view.as_ref().unwrap();
		vec![(
			Arc::downgrade(&self.attachment),
			AttachedView::Shared(view.clone()),
		)]
	}
}

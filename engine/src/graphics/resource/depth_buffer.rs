use vulkan_rs::command::frame::AttachedView;

use super::{Id, Resource};
use crate::graphics::{
	alloc, command,
	device::physical,
	flags::{self, format::Format, FormatFeatureFlags, ImageTiling},
	image::Image,
	image_view::View,
	procedure::Attachment,
	structs,
	utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder, NamedObject},
	Chain, GpuOperationBuilder,
};
use std::sync::{Arc, Weak};

pub struct DepthBufferFormatQuery {
	formats: Vec<Format>,
	tiling: ImageTiling,
}

impl Default for DepthBufferFormatQuery {
	fn default() -> Self {
		Self {
			tiling: ImageTiling::OPTIMAL,
			formats: Vec::new(),
		}
	}
}

impl DepthBufferFormatQuery {
	pub fn classic() -> Self {
		Self {
			tiling: ImageTiling::OPTIMAL,
			formats: vec![
				Format::D32_SFLOAT,
				Format::D32_SFLOAT_S8_UINT,
				Format::D24_UNORM_S8_UINT,
			],
		}
	}

	pub fn with_format_option(mut self, format: Format) -> Self {
		self.formats.push(format);
		self
	}

	pub fn with_tiling(mut self, tiling: ImageTiling) -> Self {
		self.tiling = tiling;
		self
	}

	pub fn query(self, physical: &Arc<physical::Device>) -> Result<QueryResult, NoAvailableFormat> {
		let format_flags = FormatFeatureFlags::DEPTH_STENCIL_ATTACHMENT;
		let format = physical
			.query_supported_image_formats(&self.formats, self.tiling, format_flags)
			.ok_or(NoAvailableFormat(self.formats, self.tiling, format_flags))?;
		Ok(QueryResult(format, self.tiling))
	}
}

pub struct QueryResult(Format, ImageTiling);
impl QueryResult {
	pub fn format(&self) -> Format {
		self.0
	}
	pub fn tiling(&self) -> ImageTiling {
		self.1
	}
}

#[derive(Default)]
pub struct DepthBufferBuilder {
	tiling: ImageTiling,
	attachment: Option<Arc<Attachment>>,
}
impl DepthBufferBuilder {
	pub fn with_query(mut self, query: QueryResult) -> Self {
		self.tiling = query.tiling();
		self
	}

	pub fn with_attachment(mut self, attachment: Arc<Attachment>) -> Self {
		self.attachment = Some(attachment);
		self
	}

	pub fn build(self) -> DepthBuffer {
		DepthBuffer {
			tiling: self.tiling,
			attachment: self.attachment.unwrap(),
			view: None,
		}
	}
}

pub struct DepthBuffer {
	tiling: ImageTiling,
	attachment: Arc<Attachment>,
	view: Option<Arc<View>>,
}

impl DepthBuffer {
	pub fn format_query() -> DepthBufferFormatQuery {
		DepthBufferFormatQuery::default()
	}

	pub fn classic_format_query() -> DepthBufferFormatQuery {
		DepthBufferFormatQuery::classic()
	}

	pub fn builder() -> DepthBufferBuilder {
		DepthBufferBuilder::default()
	}

	pub fn image_format(&self) -> Format {
		self.attachment.format()
	}
}

impl Resource for DepthBuffer {
	fn unique_id() -> Id {
		"DepthBuffer"
	}

	fn construct(&mut self, chain: &Chain) -> anyhow::Result<()> {
		let extent = chain.swapchain().image_extent();

		let image = Arc::new(
			Image::builder()
				.with_name(format!("{}.Image", Self::unique_id()))
				.with_alloc(
					alloc::Builder::default()
						.with_usage(flags::MemoryUsage::GpuOnly)
						.requires(flags::MemoryProperty::DEVICE_LOCAL),
				)
				.with_format(self.attachment.format())
				.with_tiling(self.tiling)
				.with_sample_count(self.attachment.sample_count())
				.with_usage(flags::ImageUsage::DEPTH_STENCIL_ATTACHMENT)
				.with_size(structs::Extent3D {
					width: extent.width,
					height: extent.height,
					depth: 1,
				})
				.build(&chain.allocator()?)?,
		);

		GpuOperationBuilder::new(image.wrap_name(|v| format!("Create({})", v)), chain)?
			.begin()?
			.format_depth_image(&image)
			.send_signal_to(chain.signal_sender())
			.end()?;

		self.view = Some(Arc::new(
			View::builder()
				.with_name(format!("{}.View", Self::unique_id()))
				.for_image(image)
				.with_view_type(flags::ImageViewType::TYPE_2D)
				.with_range(
					structs::subresource::Range::default().with_aspect(flags::ImageAspect::DEPTH),
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

#[derive(thiserror::Error, Debug)]
pub struct NoAvailableFormat(Vec<Format>, ImageTiling, FormatFeatureFlags);
impl std::fmt::Display for NoAvailableFormat {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "None of the provided formats ({:?}) are supported by the GPU with the filters: tiling={:?} flags={:?}",
			self.0, self.1, self.2
		)
	}
}

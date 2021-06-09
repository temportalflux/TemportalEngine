use crate::{
	graphics::{alloc, buffer, camera, descriptor::{self, layout::SetLayout}, flags, RenderChain},
	math::nalgebra::Vector2,
	utility::{self, AnyError},
};
use std::sync::{Arc, Weak};

pub struct Uniform {
	buffers: Vec<Arc<buffer::Buffer>>,
	descriptor_sets: Vec<Weak<descriptor::Set>>,
	descriptor_layout: Arc<SetLayout>,
}

impl Uniform {
	pub fn new(chain: &RenderChain) -> Result<Self, AnyError> {
		let descriptor_layout = Arc::new(
			SetLayout::builder()
				.with_binding(
					0,
					flags::DescriptorKind::UNIFORM_BUFFER,
					1,
					flags::ShaderKind::Vertex,
				)
				.build(&chain.logical())?,
		);

		let descriptor_sets = chain
			.persistent_descriptor_pool()
			.write()
			.unwrap()
			.allocate_descriptor_sets(&vec![descriptor_layout.clone(); chain.frame_count()])?;

		let mut buffers = Vec::new();
		for _ in 0..chain.frame_count() {
			let buffer = buffer::Buffer::builder()
				.with_usage(flags::BufferUsage::UNIFORM_BUFFER)
				.with_size(std::mem::size_of::<camera::ViewProjection>())
				.with_alloc(
					alloc::Builder::default()
						.with_usage(flags::MemoryUsage::CpuToGpu)
						.requires(flags::MemoryProperty::HOST_VISIBLE)
						.requires(flags::MemoryProperty::HOST_COHERENT),
				)
				.with_sharing(flags::SharingMode::EXCLUSIVE)
				.build(&chain.allocator())?;

			buffers.push(Arc::new(buffer));
		}
		let inst = Self {
			descriptor_layout,
			descriptor_sets,
			buffers,
		};
		let default_view_proj = camera::ViewProjection::default();
		for frame in 0..chain.frame_count() {
			inst.write_data(frame, &default_view_proj)?;
		}
		Ok(inst)
	}

	pub fn write_descriptor_sets(&self, chain: &RenderChain) {
		use alloc::Object;
		use descriptor::update::*;
		let mut set_updates = Queue::default();
		for (set_weak, buffer_rc) in self.descriptor_sets.iter().zip(self.buffers.iter()) {
			set_updates = set_updates.with(Operation::Write(WriteOp {
				destination: Descriptor {
					set: set_weak.clone(),
					binding_index: 0,
					array_element: 0,
				},
				kind: flags::DescriptorKind::UNIFORM_BUFFER,
				object: ObjectKind::Buffer(vec![BufferKind {
					buffer: buffer_rc.clone(),
					offset: 0,
					range: buffer_rc.size(),
				}]),
			}));
		}
		set_updates.apply(&chain.logical());
	}

	pub fn layout(&self) -> &Arc<SetLayout> {
		&self.descriptor_layout
	}

	pub fn get_set(&self, frame: usize) -> Option<Arc<descriptor::Set>> {
		self.descriptor_sets[frame].upgrade()
	}

	fn write_data(&self, frame: usize, camera: &camera::ViewProjection) -> utility::Result<()> {
		let mut mem = self.buffers[frame].memory()?;
		let wrote_all = mem
			.write_item(camera)
			.map_err(|e| utility::Error::GraphicsBufferWrite(e))?;
		assert!(wrote_all);
		Ok(())
	}

	pub fn write_camera(
		&self,
		frame: usize,
		resolution: &Vector2<f32>,
		camera: &camera::Camera,
	) -> utility::Result<()> {
		self.write_data(frame, &camera.as_uniform_matrix(resolution))
	}
}

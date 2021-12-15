use crate::{
	graphics::{
		alloc, buffer, camera,
		descriptor::{self, layout::SetLayout},
		flags,
		utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder},
		RenderChain,
	},
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
	pub fn new<TData, TStr>(name: TStr, chain: &RenderChain) -> Result<Self, AnyError>
	where
		TStr: Into<String>,
		TData: Default + Sized,
	{
		let uniform_name: String = name.into();

		let descriptor_layout = Arc::new(
			SetLayout::builder()
				.with_name(format!("{}.DescriptorLayout", uniform_name))
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
			.allocate_named_descriptor_sets(
				&(0..chain.frame_count())
					.map(|i| {
						(
							descriptor_layout.clone(),
							Some(format!("{}.Frame{}.Descriptor", uniform_name, i)),
						)
					})
					.collect(),
			)?;

		let mut buffers = Vec::new();
		for i in 0..chain.frame_count() {
			let buffer = buffer::Buffer::builder()
				.with_name(format!("{}.Frame{}", uniform_name, i))
				.with_usage(flags::BufferUsage::UNIFORM_BUFFER)
				.with_size(std::mem::size_of::<TData>())
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
		let default_view_proj = TData::default();
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

	pub fn write_data<TData>(&self, frame: usize, camera: &TData) -> utility::Result<()>
	where
		TData: Sized,
	{
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

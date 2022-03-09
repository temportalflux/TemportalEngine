use crate::graphics::{
	alloc, buffer,
	descriptor::{self, layout::SetLayout},
	device::logical,
	flags,
	utility::{BuildFromAllocator, BuildFromDevice, NameableBuilder},
};
use anyhow::Result;
use std::sync::{Arc, RwLock, Weak};

pub struct Uniform {
	buffers: Vec<Arc<buffer::Buffer>>,
	descriptor_sets: Vec<Weak<descriptor::Set>>,
	descriptor_layout: Arc<SetLayout>,
}

impl Uniform {
	pub fn new<TData, TStr>(
		name: TStr,
		logical: &Arc<logical::Device>,
		allocator: &Arc<alloc::Allocator>,
		descriptor_pool: &Arc<RwLock<descriptor::Pool>>,
		view_count: usize,
	) -> Result<Self>
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
				.build(logical)?,
		);

		let descriptor_sets = descriptor_pool
			.write()
			.unwrap()
			.allocate_named_descriptor_sets(
				&(0..view_count)
					.map(|i| {
						(
							descriptor_layout.clone(),
							Some(format!("{}.Frame{}.Descriptor", uniform_name, i)),
						)
					})
					.collect(),
			)?;

		let mut buffers = Vec::new();
		for i in 0..view_count {
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
				.build(allocator)?;

			buffers.push(Arc::new(buffer));
		}
		let inst = Self {
			descriptor_layout,
			descriptor_sets,
			buffers,
		};
		let default_view_proj = TData::default();
		for frame in 0..view_count {
			inst.write_data(frame, &default_view_proj)?;
		}
		Ok(inst)
	}

	pub fn write_descriptor_sets(&self, logical: &logical::Device) {
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
		set_updates.apply(logical);
	}

	pub fn layout(&self) -> &Arc<SetLayout> {
		&self.descriptor_layout
	}

	pub fn get_set(&self, frame: usize) -> Option<Arc<descriptor::Set>> {
		self.descriptor_sets[frame].upgrade()
	}

	pub fn write_data<TData>(&self, frame: usize, data: &TData) -> Result<()>
	where
		TData: Sized,
	{
		let mut mem = self.buffers[frame].memory()?;
		let wrote_all = mem.write_item(data)?;
		assert!(wrote_all);
		Ok(())
	}
}

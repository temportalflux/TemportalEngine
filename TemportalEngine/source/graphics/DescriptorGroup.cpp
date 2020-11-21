#include "graphics/DescriptorGroup.hpp"

#include "graphics/DescriptorPool.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

DescriptorGroup::DescriptorGroup()
	: mSetCount(0)
{
	this->setArchetypeAmount(1);
}

DescriptorGroup& DescriptorGroup::setArchetypeAmount(uSize amt)
{
	this->mArchetypes.resize(amt);
	return *this;
}

DescriptorGroup& DescriptorGroup::setAmount(ui32 setCount)
{
	this->mSetCount = setCount;
	for (auto& archetype : this->mArchetypes)
	{
		archetype.mWriteInstructions.clear();
		archetype.mWriteInstructions.resize(setCount);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::setBindingCount(uSize count)
{
	this->mBindings.clear();
	this->mBindings.resize(count);
	this->mBindingIdxById.clear();
	return *this;
}

DescriptorGroup& DescriptorGroup::addBinding(
	std::string const &id, uIndex const idx,
	graphics::DescriptorType const type,
	graphics::ShaderStage const shaderStage,
	ui32 count /* = 1 */
)
{
	assert(idx < this->mBindings.size());
	this->mBindingIdxById.insert(std::make_pair(id, idx));
	this->mBindings[idx] = vk::DescriptorSetLayoutBinding()
		.setBinding((ui32)idx)
		.setDescriptorType(type.as<vk::DescriptorType>())
		.setStageFlags(shaderStage.as<vk::ShaderStageFlagBits>())
		.setDescriptorCount(count);
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	std::string const &id,
	graphics::Buffer &buffer,
	uIndex idxArchetype
)
{
	auto const idx = this->mBindingIdxById[id];
	for (auto& writeInstructionSet : this->mArchetypes[idxArchetype].mWriteInstructions)
	{
		auto& writeInfo = writeInstructionSet.pushBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(buffer.get()))
			.setOffset(0) // offset from the previous item. must be a multiple of a property in the graphics device
			.setRange(buffer.getSize());

		writeInstructionSet.writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding((ui32)idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPBufferInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	std::string const &id,
	std::vector<graphics::Buffer*> &buffers,
	uIndex idxArchetype
)
{
	auto const idx = this->mBindingIdxById[id];
	for (uIndex i = 0; i < this->mSetCount; ++i)
	{
		auto& writeInfo = this->mArchetypes[idxArchetype].mWriteInstructions[i].pushBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(buffers[i]->get()))
			.setOffset(0) // offset from the previous item. must be a multiple of a property in the graphics device
			.setRange(buffers[i]->getSize());

		this->mArchetypes[idxArchetype].mWriteInstructions[i].writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding((ui32)idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPBufferInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	std::string const &id,
	vk::ImageLayout const layout, ImageView *view, ImageSampler *sampler,
	uIndex idxArchetype
)
{
	if (view == nullptr || sampler == nullptr) return *this;
	auto const idx = this->mBindingIdxById[id];
	for (auto& writeInstructionSet : this->mArchetypes[idxArchetype].mWriteInstructions)
	{
		auto& writeInfo = writeInstructionSet.pushImageInfo()
			.setImageLayout(layout)
			.setImageView(*reinterpret_cast<vk::ImageView*>(view->get()))
			.setSampler(*reinterpret_cast<vk::Sampler*>(sampler->get()));

		writeInstructionSet.writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding((ui32)idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPImageInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::create(std::shared_ptr<GraphicsDevice> device, DescriptorPool *pool)
{
	this->mInternalLayout = device->createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((ui32)this->mBindings.size())
		.setPBindings(this->mBindings.data())
	);

	// will be deallocated when the pool is destroyed
	for (auto& archetype : this->mArchetypes)
	{
		archetype.mInternalSets = device->allocateDescriptorSets(pool, this, this->mSetCount);
	}

	return *this;
}

DescriptorGroup& DescriptorGroup::writeAttachments(std::shared_ptr<GraphicsDevice> device)
{
	for (auto& archetype : this->mArchetypes)
	{
		for (uIndex i = 0; i < this->mSetCount; ++i)
		{
			auto writes = std::vector<vk::WriteDescriptorSet>(archetype.mWriteInstructions[i].writes);
			for (auto& write : writes)
			{
				write.setDstSet(archetype.mInternalSets[i]);
			}
			device->updateDescriptorSets(writes);
		}
	}
	return *this;
}

void DescriptorGroup::invalidate()
{
	this->mInternalLayout.reset();
}

vk::DescriptorSetLayout DescriptorGroup::layout() const
{
	return this->mInternalLayout.get();
}

vk::DescriptorSet const& DescriptorGroup::getDescriptorSet(uIndex idxSet, uIndex idxArchetype) const
{
	assert(idxArchetype < this->mArchetypes.size());
	assert(idxSet < this->mArchetypes[idxArchetype].mInternalSets.size());
	return this->mArchetypes[idxArchetype].mInternalSets[idxSet];
}

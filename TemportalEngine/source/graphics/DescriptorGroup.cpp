#include "graphics/DescriptorGroup.hpp"

#include "graphics/DescriptorPool.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

DescriptorGroup& DescriptorGroup::setBindingCount(uSize count)
{
	this->mBindings.resize(count);
	return *this;
}

DescriptorGroup& DescriptorGroup::setAmount(ui32 setCount)
{
	this->mSetCount = setCount;
	this->mWriteInstructions.resize(setCount);
	return *this;
}

DescriptorGroup& DescriptorGroup::addBinding(
	ui32 const idx,
	vk::DescriptorType const type,
	vk::ShaderStageFlags const shaderStage,
	ui32 count /* = 1 */
)
{
	assert(idx < this->mBindings.size());
	this->mBindings[idx] = vk::DescriptorSetLayoutBinding()
		.setBinding(idx)
		.setDescriptorType(type).setStageFlags(shaderStage)
		.setDescriptorCount(count);
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	ui32 const idx,
	graphics::Buffer &buffer, ui32 const offset
)
{
	for (auto& writeInstructionSet : this->mWriteInstructions)
	{
		auto& writeInfo = writeInstructionSet.pushBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(buffer.get()))
			.setOffset(offset)
			.setRange(buffer.getSize());

		writeInstructionSet.writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding(idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPBufferInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	ui32 const idx,
	std::vector<graphics::Buffer> &buffers, ui32 const offset
)
{
	for (uIndex i = 0; i < this->mSetCount; ++i)
	{
		auto& writeInfo = this->mWriteInstructions[i].pushBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(buffers[i].get()))
			.setOffset(offset)
			.setRange(buffers[i].getSize());

		this->mWriteInstructions[i].writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding(idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPBufferInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::attachToBinding(
	ui32 const idx,
	vk::ImageLayout const layout, ImageView *view, ImageSampler *sampler
)
{
	if (view == nullptr || sampler == nullptr) return *this;
	for (auto& writeInstructionSet : this->mWriteInstructions)
	{
		auto& writeInfo = writeInstructionSet.pushImageInfo()
			.setImageLayout(layout)
			.setImageView(*reinterpret_cast<vk::ImageView*>(view->get()))
			.setSampler(*reinterpret_cast<vk::Sampler*>(sampler->get()));

		writeInstructionSet.writes.push_back(
			vk::WriteDescriptorSet()
			// The destination set is handled when the write is actually written
			//.setDstSet(INTERNAL_SET)
			.setDstBinding(idx)
			.setDescriptorType(this->mBindings[idx].descriptorType)
			.setDescriptorCount(this->mBindings[idx].descriptorCount)
			.setDstArrayElement(0)
			.setPImageInfo(&writeInfo)
		);
	}
	return *this;
}

DescriptorGroup& DescriptorGroup::create(LogicalDevice *device, DescriptorPool *pool)
{
	this->mInternalLayout = device->mDevice->createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((ui32)this->mBindings.size())
		.setPBindings(this->mBindings.data())
	);

	std::vector<vk::DescriptorSetLayout> layouts(this->mSetCount, this->mInternalLayout.get());
	// will be deallocated when the pool is destroyed
	this->mInternalSets = device->mDevice->allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(extract<vk::DescriptorPool>(pool))
		.setDescriptorSetCount(this->mSetCount)
		.setPSetLayouts(layouts.data())
	);

	return *this;
}

DescriptorGroup& DescriptorGroup::writeAttachments(LogicalDevice *device)
{
	for (uIndex i = 0; i < this->mSetCount; ++i)
	{
		auto writes = std::vector<vk::WriteDescriptorSet>(this->mWriteInstructions[i].writes);
		for (auto& write : writes)
		{
			write.setDstSet(this->mInternalSets[i]);
		}
		device->mDevice->updateDescriptorSets((ui32)writes.size(), writes.data(), 0, nullptr);
	}
	return *this;
}

void DescriptorGroup::invalidate()
{
	this->mInternalSets.clear();
	this->mInternalLayout.reset();
}

vk::DescriptorSetLayout DescriptorGroup::layout() const
{
	return this->mInternalLayout.get();
}

const vk::DescriptorSet& DescriptorGroup::operator[](uIndex idxSet) const
{
	assert(idxSet < this->mInternalSets.size());
	return this->mInternalSets[idxSet];
}

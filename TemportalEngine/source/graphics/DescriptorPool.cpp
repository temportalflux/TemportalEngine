#include "graphics/DescriptorPool.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

DescriptorPool::DescriptorPool() : DeviceObject()
{
}

DescriptorPool& DescriptorPool::setFlags(vk::DescriptorPoolCreateFlags flags)
{
	this->mFlags = flags;
	return *this;
}

DescriptorPool& DescriptorPool::setAllocationMultiplier(ui32 const setCount)
{
	this->mAllocationMultiplier = setCount;
	return *this;
}

DescriptorPool& DescriptorPool::setPoolSize(ui32 maxSets, std::unordered_map<vk::DescriptorType, ui32> const &sizes)
{
	this->mMaxSets = maxSets;
	this->mAvailableAllocationsPerType = sizes;
	return *this;
}

void DescriptorPool::create()
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>(this->mAvailableAllocationsPerType.size());
	std::transform(
		this->mAvailableAllocationsPerType.begin(),
		this->mAvailableAllocationsPerType.end(),
		poolSizes.begin(), [this](auto &pair) {
		return vk::DescriptorPoolSize()
			.setType(pair.first)
			.setDescriptorCount(this->mAllocationMultiplier * pair.second);
	});

	this->mInternal = this->device()->createDescriptorPool(
		vk::DescriptorPoolCreateInfo()
		.setFlags(this->mFlags)
		.setPoolSizeCount((ui32)poolSizes.size()).setPPoolSizes(poolSizes.data())
		.setMaxSets(this->mMaxSets)
	);
}

void* DescriptorPool::get()
{
	return &this->mInternal.get();
}

void DescriptorPool::invalidate()
{
	this->mInternal.reset();
}

void DescriptorPool::resetConfiguration()
{
	this->mMaxSets = 0;
	this->mAvailableAllocationsPerType.clear();
	this->mAllocationMultiplier = 0;
}

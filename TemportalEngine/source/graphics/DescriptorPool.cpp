#include "graphics/DescriptorPool.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

void DescriptorPool::setPoolSize(std::unordered_map<vk::DescriptorType, ui32> const &sizes)
{
	this->mAvailableAllocationsPerType = sizes;
}

void DescriptorPool::create(LogicalDevice *device, ui32 const &frameCount)
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>(this->mAvailableAllocationsPerType.size());
	std::transform(
		this->mAvailableAllocationsPerType.begin(),
		this->mAvailableAllocationsPerType.end(),
		poolSizes.begin(), [frameCount](auto &pair) {
		return vk::DescriptorPoolSize()
			.setType(pair.first)
			.setDescriptorCount(frameCount * pair.second);
	});

	this->mInternal = device->mDevice->createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount((ui32)poolSizes.size()).setPPoolSizes(poolSizes.data())
		.setMaxSets(frameCount)
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

#include "graphics/DescriptorPool.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

DescriptorPool& DescriptorPool::setFlags(vk::DescriptorPoolCreateFlags flags)
{
	this->mFlags = flags;
	return *this;
}

DescriptorPool& DescriptorPool::setPoolSize(ui32 maxSets, std::unordered_map<vk::DescriptorType, ui32> const &sizes)
{
	this->mMaxSets = maxSets;
	this->mAvailableAllocationsPerType = sizes;
	return *this;
}

DescriptorPool& DescriptorPool::create(std::shared_ptr<GraphicsDevice> device, ui32 const &frameCount)
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

	this->mInternal = device->createDescriptorPool(
		vk::DescriptorPoolCreateInfo()
		.setFlags(this->mFlags)
		.setPoolSizeCount((ui32)poolSizes.size()).setPPoolSizes(poolSizes.data())
		.setMaxSets(this->mMaxSets)
	);
	return *this;
}

void* DescriptorPool::get()
{
	return &this->mInternal.get();
}

void DescriptorPool::invalidate()
{
	this->mInternal.reset();
}

#include "graphics/MemoryBacked.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

MemoryBacked& MemoryBacked::setMemoryRequirements(vk::MemoryPropertyFlags flags)
{
	this->mMemoryFlags = flags;
	return *this;
}

uSize MemoryBacked::getMemorySize() const
{
	return this->mMemorySize;
}

void MemoryBacked::createMemory(std::shared_ptr<GraphicsDevice> device, vk::MemoryRequirements const &req)
{
	auto memoryType = this->findMemoryType(device, req.memoryTypeBits, this->mMemoryFlags);
	assert(memoryType.has_value());

	// Allocates memory on the physical device/GPU for the buffer
	this->mMemorySize = req.size;
	this->mBufferMemory = device->allocateMemory(
		vk::MemoryAllocateInfo()
		.setAllocationSize(this->mMemorySize)
		.setMemoryTypeIndex(memoryType.value())
	);

	// NOTE: Multiple objects could share the same memory. Somehow, this class should account for that
	this->bind(device, /*memory offset*/ 0);
}

std::optional<ui32> MemoryBacked::findMemoryType(std::shared_ptr<GraphicsDevice> device, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
	auto memoryProperties = device->physical().getMemoryProperties();
	for (ui32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		bool bHasFilter = typeFilter & (1 << i);
		bool bHasFlags = (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags;
		if (bHasFilter && bHasFlags)
		{
			return i;
		}
	}
	return std::nullopt;
}

void MemoryBacked::write(std::shared_ptr<GraphicsDevice> device, ui64 offset, void* src, ui64 size)
{
	void* dest = device->mapMemory(this, offset, this->mMemorySize);
	memcpy(dest, src, size);
	device->unmapMemory(this);
}

void MemoryBacked::invalidate()
{
	this->mBufferMemory.reset();
}

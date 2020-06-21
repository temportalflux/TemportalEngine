#include "graphics/MemoryBacked.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/PhysicalDevice.hpp"

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

void MemoryBacked::createMemory(LogicalDevice const *pDevice, vk::MemoryRequirements const &req)
{
	auto memoryType = this->findMemoryType(pDevice->mpPhysicalDevice, req.memoryTypeBits, this->mMemoryFlags);
	assert(memoryType.has_value());

	// Allocates memory on the physical device/GPU for the buffer
	this->mMemorySize = req.size;
	auto allocInfo = vk::MemoryAllocateInfo().setAllocationSize(this->mMemorySize).setMemoryTypeIndex(memoryType.value());
	this->mBufferMemory = pDevice->mDevice->allocateMemoryUnique(allocInfo);

	// NOTE: Multiple objects could share the same memory. Somehow, this class should account for that
	this->bind(pDevice, this->mBufferMemory.get(), /*memory offset*/ 0);
}

std::optional<ui32> MemoryBacked::findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
	auto memoryProperties = pDevice->getMemoryProperties();
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

void MemoryBacked::write(LogicalDevice const *pDevice, uSize offset, void* src, uSize size)
{
	void* dest = pDevice->mDevice->mapMemory(this->mBufferMemory.get(), offset, this->mMemorySize, /*flags*/{});
	memcpy(dest, src, size);
	pDevice->mDevice->unmapMemory(this->mBufferMemory.get());
}

void MemoryBacked::invalidate()
{
	this->mBufferMemory.reset();
}

#include "graphics/Memory.hpp"

#include "graphics/GraphicsDevice.hpp"

using namespace graphics;

Memory::~Memory()
{
	this->destroy();
}

Memory& Memory::setFlags(vk::MemoryPropertyFlags flags)
{
	this->mFlags = flags;
	return *this;
}

Memory& Memory::configureSlot(vk::MemoryRequirements const &requirements, uIndex &outSlotIndex)
{
	if (this->mMemoryTypeBits) assert(requirements.memoryTypeBits == this->mMemoryTypeBits);
	else this->mMemoryTypeBits = requirements.memoryTypeBits;

	outSlotIndex = this->mSlots.size();
	this->mSlots.push_back({ this->mTotalSize, requirements.size });
	this->mTotalSize += requirements.size;

	return *this;
}

std::optional<ui32> findMemoryType(std::shared_ptr<GraphicsDevice> device, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags)
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

Memory& Memory::create(std::shared_ptr<GraphicsDevice> device)
{
	assert(this->mTotalSize > 0 && this->mMemoryTypeBits);

	auto memoryType = findMemoryType(device, *this->mMemoryTypeBits, this->mFlags);
	assert(memoryType);

	this->mInternal = device->allocateMemory(
		vk::MemoryAllocateInfo()
		.setAllocationSize(this->mTotalSize)
		.setMemoryTypeIndex(*memoryType)
	);

	return *this;
}

void Memory::destroy()
{
	this->mInternal.reset();
}

Memory& Memory::bind(std::shared_ptr<GraphicsDevice> device, uIndex const idxSlot, Buffer const *buffer)
{
	device->bindMemory(this, buffer, this->mSlots[idxSlot].offset);
	return *this;
}

Memory& Memory::bind(std::shared_ptr<GraphicsDevice> device, uIndex const idxSlot, Image const *image)
{
	device->bindMemory(this, image, this->mSlots[idxSlot].offset);
	return *this;
}

void Memory::write(std::shared_ptr<GraphicsDevice> device, ui64 offset, void* src, uSize size)
{
	void* dest = device->mapMemory(this, offset, this->mTotalSize);
	memcpy(dest, src, size);
	device->unmapMemory(this);
}

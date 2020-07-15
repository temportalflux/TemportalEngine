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

void Memory::create()
{
	assert(this->mTotalSize > 0 && this->mMemoryTypeBits);

	auto memoryType = findMemoryType(this->device(), *this->mMemoryTypeBits, this->mFlags);
	assert(memoryType);

	this->mInternal = this->device()->allocateMemory(
		vk::MemoryAllocateInfo()
		.setAllocationSize(this->mTotalSize)
		.setMemoryTypeIndex(*memoryType)
	);
}

void* Memory::get()
{
	return &this->mInternal.get();
}

void Memory::invalidate()
{
	this->mInternal.reset();
}

void Memory::resetConfiguration()
{
	this->mFlags = {};
}

Memory& Memory::bind(uIndex const idxSlot, Buffer const *buffer)
{
	assert(this->mInternal);
	this->device()->bindMemory(this, buffer, this->mSlots[idxSlot].offset);
	return *this;
}

Memory& Memory::bind(uIndex const idxSlot, Image const *image)
{
	assert(this->mInternal);
	this->device()->bindMemory(this, image, this->mSlots[idxSlot].offset);
	return *this;
}

void Memory::writeInternal(ui64 const mapOffset, ui64 const mapSize, uSize const dataOffset, void* src, uSize size, bool bClear)
{
	assert(this->mInternal);
	void* dest = this->device()->mapMemory(this, mapOffset, mapSize);
	void* destOffsetted = (void*)(((uSize)dest) + dataOffset);
	if (bClear) memset(destOffsetted, 0, mapSize);
	memcpy(destOffsetted, src, size);
	this->device()->unmapMemory(this);
}

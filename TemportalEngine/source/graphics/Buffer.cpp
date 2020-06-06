#include "graphics/Buffer.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

Buffer::Buffer(Buffer &&other)
{
	*this = std::move(other);
}

Buffer& Buffer::operator=(Buffer &&other)
{
	this->mUsageFlags = other.mUsageFlags;
	this->mMemoryFlags = other.mMemoryFlags;
	this->mSize = other.mSize;
	this->mInternal.swap(other.mInternal);
	this->mBufferMemory.swap(other.mBufferMemory);
	other.destroy();
	return *this;
}

Buffer& Buffer::setUsage(vk::BufferUsageFlags flags)
{
	this->mUsageFlags = flags;
	return *this;
}

Buffer& Buffer::setMemoryRequirements(vk::MemoryPropertyFlags flags)
{
	this->mMemoryFlags = flags;
	return *this;
}

Buffer& Buffer::setSize(ui64 size)
{
	this->mSize = size;
	return *this;
}

ui64 Buffer::getSize() const
{
	return this->mSize;
}

void* Buffer::get()
{
	return &this->mInternal.get();
}

void Buffer::create(LogicalDevice const *pDevice)
{
	this->mInternal = pDevice->mDevice->createBufferUnique(
		vk::BufferCreateInfo()
		.setUsage(this->mUsageFlags)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize(this->mSize)
	);

	auto memRequirements = pDevice->mDevice->getBufferMemoryRequirements(this->mInternal.get());
	auto memoryType = this->findMemoryType(pDevice->mpPhysicalDevice, memRequirements.memoryTypeBits, this->mMemoryFlags);
	assert(memoryType.has_value());

	// Allocates memory on the physical device/GPU for the buffer
	auto allocInfo = vk::MemoryAllocateInfo().setAllocationSize(memRequirements.size).setMemoryTypeIndex(memoryType.value());
	this->mBufferMemory = pDevice->mDevice->allocateMemoryUnique(allocInfo);

	// memory offset could probably be used to offset the buffers usage of memory by some amount of bits, should that ever be needed
	pDevice->mDevice->bindBufferMemory(this->mInternal.get(), this->mBufferMemory.get(), /*memory offset*/ 0);
}

std::optional<ui32> Buffer::findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags)
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

void Buffer::write(LogicalDevice const *pDevice, ui64 offset, void* src, ui64 size)
{
	void* dest = pDevice->mDevice->mapMemory(this->mBufferMemory.get(), offset, this->mSize, /*flags*/ {});
	memcpy(dest, src, size);
	pDevice->mDevice->unmapMemory(this->mBufferMemory.get());
}

void Buffer::destroy()
{
	this->mInternal.reset();
	this->mBufferMemory.reset();
}

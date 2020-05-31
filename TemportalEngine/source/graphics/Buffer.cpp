#include "graphics/Buffer.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

Buffer& Buffer::setSize(ui64 size)
{
	this->mSize = size;
	return *this;
}

void* Buffer::get()
{
	return &this->mInternal.get();
}

void Buffer::create(LogicalDevice const *pDevice)
{
	this->mpDevice = pDevice;

	this->mInternal = pDevice->mDevice->createBufferUnique(
		vk::BufferCreateInfo()
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize(this->mSize)
	);

	auto memRequirements = pDevice->mDevice->getBufferMemoryRequirements(this->mInternal.get());
	auto memoryType = this->findMemoryType(pDevice->mpPhysicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
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

void Buffer::write_internal(void* src, ui32 size)
{
	void* dest = this->mpDevice->mDevice->mapMemory(this->mBufferMemory.get(), /* memory offset*/ 0, this->mSize, /*flags*/ {});
	memcpy(dest, src, size);
	this->mpDevice->mDevice->unmapMemory(this->mBufferMemory.get());
}

void Buffer::destroy()
{
	this->mpDevice = nullptr;
	this->mInternal.reset();
	this->mBufferMemory.reset();
}

#include "graphics/Buffer.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

Buffer::Buffer(Buffer &&other)
{
	*this = std::move(other);
}

Buffer& Buffer::operator=(Buffer &&other)
{
	this->mUsageFlags = other.mUsageFlags;
	this->mSize = other.mSize;
	this->mInternal.swap(other.mInternal);

	this->mMemoryFlags = other.mMemoryFlags;
	this->mMemorySize = other.mMemorySize;
	this->mBufferMemory.swap(other.mBufferMemory);
	
	other.destroy();
	return *this;
}

Buffer& Buffer::setUsage(vk::BufferUsageFlags flags)
{
	this->mUsageFlags = flags;
	return *this;
}

Buffer& Buffer::setSize(uSize size)
{
	this->mSize = size;
	return *this;
}

uSize Buffer::getSize() const
{
	return this->mSize;
}

void* Buffer::get()
{
	return &this->mInternal.get();
}

void Buffer::create(LogicalDevice *pDevice)
{
	vk::Device& device = extract<vk::Device>(pDevice);

	this->mInternal = device.createBufferUnique(
		vk::BufferCreateInfo()
		.setUsage(this->mUsageFlags)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize((ui64)this->mSize)
	);

	auto memRequirements = device.getBufferMemoryRequirements(this->mInternal.get());
	this->createMemory(pDevice, memRequirements);
}

void Buffer::bind(LogicalDevice *pDevice, vk::DeviceMemory &mem, uSize offset)
{
	extract<vk::Device>(pDevice).bindBufferMemory(this->mInternal.get(), mem, offset);
}

void Buffer::destroy()
{
	this->mInternal.reset();
	MemoryBacked::invalidate();
}

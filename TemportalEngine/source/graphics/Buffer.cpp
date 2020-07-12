#include "graphics/Buffer.hpp"

#include "graphics/GameRenderer.hpp"
#include "graphics/GraphicsDevice.hpp"
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

void Buffer::create(std::shared_ptr<GraphicsDevice> device)
{
	this->mInternal = device->createBuffer(
		vk::BufferCreateInfo()
		.setUsage(this->mUsageFlags)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize((ui64)this->mSize)
	);

	this->createMemory(device, device->getMemoryRequirements(this));
}

void Buffer::bind(std::shared_ptr<GraphicsDevice> device, ui64 offset)
{
	device->bindMemory(this, this, offset);
}

void Buffer::destroy()
{
	this->mInternal.reset();
	MemoryBacked::invalidate();
}

void Buffer::writeBuffer(GameRenderer *renderer, uSize offset, void* data, uSize size)
{
	Buffer& stagingBuffer = Buffer()
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setSize(size);
	stagingBuffer.setMemoryRequirements(
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);
	stagingBuffer.create(renderer->getDevice());
	stagingBuffer.write(renderer->getDevice(), offset, data, size);
	{
		auto buffers = renderer->getTransientPool().createCommandBuffers(1);
		buffers[0]
			.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.copyBuffer(&stagingBuffer, this, size)
			.end();
		auto queue = renderer->getDevice()->getQueue(QueueFamily::Enum::eGraphics);
		queue.submit(
			vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
			vk::Fence()
		);
		queue.waitIdle();
	}
	stagingBuffer.destroy();
}

#include "graphics/Buffer.hpp"

#include "graphics/Command.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/Memory.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

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

void Buffer::create()
{
	OPTICK_EVENT();
	this->mInternal = this->device()->createBuffer(
		vk::BufferCreateInfo()
		.setUsage(this->mUsageFlags)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize((ui64)this->mSize)
	);
}

void* Buffer::get()
{
	return &this->mInternal.get();
}

void Buffer::invalidate()
{
	this->mInternal.reset();
}

void Buffer::resetConfiguration()
{
	this->mUsageFlags = {};
	this->mSize = 0;
}

vk::MemoryRequirements Buffer::getRequirements() const
{
	return this->device()->getMemoryRequirements(this);
}

void Buffer::bindMemory()
{
	this->memory()->bind(this->memorySlot(), this);
}

void Buffer::writeBuffer(CommandPool* transientPool, uSize offset, void* data, uSize size, bool bClear)
{
	OPTICK_EVENT();
	Buffer::writeDataToGPU(
		this->device(), transientPool,
		this->getSize(), bClear,
		offset, data, size,
		[this, size](Command* cmd, Buffer *stagingBuffer) {
			cmd->copyBuffer(stagingBuffer, this, size);
		}
	);
}

void Buffer::write(uSize const offset, void* src, uSize size, bool bClear)
{
	OPTICK_EVENT();
	this->memory()->write(this->memorySlot(), offset, src, size, bClear);
}

void Buffer::writeDataToGPU(
	std::shared_ptr<GraphicsDevice> device,
	CommandPool* transientPool,
	uSize const bufferSize, bool const bClear,
	uSize const dataOffset, void* data, uSize const dataSize,
	std::function<void(Command* cmd, Buffer* stagingBuffer)> writeCommands
)
{
	OPTICK_EVENT();
	// Setup a buffer to be written to before its sent to GPU
	Buffer stagingBuffer;
	stagingBuffer.setDevice(device);
	stagingBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setSize(bufferSize);
	stagingBuffer.create();

	// Setup a memory chunk which can be written to by CPU and copyable to GPU
	Memory stagingMemory;
	stagingMemory.setDevice(device);
	stagingMemory
		.setFlags(
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		);

	// Configure memory with buffer needs
	uIndex i;
	stagingMemory.configureSlot(stagingBuffer.getRequirements(), i);
	// Create the memory, now fully configured
	stagingMemory.create();
	// Link the buffer and allocated memory
	stagingMemory.bind(i, &stagingBuffer);

	// Perform write
	stagingMemory.write(i, dataOffset, data, dataSize, bClear);

	/*
		TODO: submitOneOff contains a waitIdle at the end of the call. This causes all commands to be synchronous.
		For higher throughput, waitIdle should not be called, and the command buffers should rely entirely
		on the pipeline barriers.
		This means multiple images or buffers could be written to GPU at once.
	*/
	// Copy buffer to GPU
	transientPool->submitOneOff([&writeCommands, &stagingBuffer](Command* cmd) {
		writeCommands(cmd, &stagingBuffer);
	});

	// Destroy buffer and GPU memory
	stagingBuffer.destroy();
	stagingMemory.destroy();
}

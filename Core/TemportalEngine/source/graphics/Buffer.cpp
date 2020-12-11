#include "graphics/Buffer.hpp"

#include "graphics/Command.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

Buffer::Buffer(Buffer &&other)
{
	*this = std::move(other);
}

Buffer& Buffer::operator=(Buffer &&other)
{
	this->setDevice(other.device());

	this->mUsageFlags = other.mUsageFlags;
	this->mSize = other.mSize;

	this->mMemoryUsageFlag = other.mMemoryUsageFlag;
	this->mAllocated = std::move(other.mAllocated);
	this->mAllocHandle = other.mAllocHandle;
	other.mAllocHandle = nullptr;
	
	return *this;
}

Buffer::~Buffer()
{
	destroy();
}

Buffer& Buffer::setUsage(vk::BufferUsageFlags flags, MemoryUsage memUsage)
{
	this->mUsageFlags = flags;
	this->mMemoryUsageFlag = memUsage;
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
	this->mAllocHandle = this->device()->getVMA()->createBuffer(
		vk::BufferCreateInfo()
		.setUsage(this->mUsageFlags)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize((ui64)this->mSize),
		this->mMemoryUsageFlag, this->mAllocated
	);
}

void* Buffer::get() { return &this->mAllocated; }
void* Buffer::get() const { return (void*)(&this->mAllocated); }

void Buffer::invalidate()
{
	if (this->mAllocHandle)
	{
		this->device()->getVMA()->destroyBuffer(this->mAllocated, this->mAllocHandle);
		this->mAllocHandle = nullptr;
	}
}

void Buffer::resetConfiguration()
{
	this->mUsageFlags = {};
	this->mSize = 0;
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
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc, MemoryUsage::eCPUSource)
		.setSize(bufferSize);
	stagingBuffer.create();

	// Perform write
	stagingBuffer.write(dataOffset, data, dataSize, bClear);

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
}

void Buffer::write(uSize const offset, void* src, uSize size, bool bClear)
{
	OPTICK_EVENT();
	auto vma = this->device()->getVMA();
	auto ptr = vma->mapMemory(this->mAllocHandle);
	void* destOffsetted = (void*)(uIndex(ptr) + offset);
	if (bClear) memset(destOffsetted, 0, size);
	memcpy(destOffsetted, src, size);
	vma->unmapMemory(this->mAllocHandle);
}

#include "graphics/Image.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

Image::Image(Image &&other)
{
	*this = std::move(other);
}

Image& Image::operator=(Image &&other)
{
	this->setDevice(other.device());

	this->mType = other.mType;
	this->mFormat = other.mFormat;
	this->mTiling = other.mTiling;
	this->mUsage = other.mUsage;
	this->mImageSize = other.mImageSize;

	this->mMemoryUsageFlag = other.mMemoryUsageFlag;
	this->mAllocated = std::move(other.mAllocated);
	this->mAllocHandle = other.mAllocHandle;
	other.mAllocHandle = nullptr;

	return *this;
}

Image::~Image()
{
	destroy();
}

Image& Image::setFormat(vk::Format format)
{
	this->mFormat = format;
	return *this;
}

vk::Format Image::getFormat() const
{
	return this->mFormat;
}

bool Image::hasStencilComponent() const
{
	return this->mFormat == vk::Format::eD32SfloatS8Uint || this->mFormat == vk::Format::eD24UnormS8Uint;
}

Image& Image::setTiling(vk::ImageTiling tiling)
{
	this->mTiling = tiling;
	return *this;
}

Image& Image::setUsage(vk::ImageUsageFlags usage)
{
	this->mUsage = usage;
	this->mMemoryUsageFlag = graphics::MemoryUsage::eGPUOnly;
	return *this;
}

Image& Image::setSize(math::Vector3UInt const &size)
{
	this->mImageSize = size;
	return *this;
}

math::Vector3UInt Image::getSize() const
{
	return this->mImageSize;
}

void Image::create()
{
	OPTICK_EVENT()
	this->mAllocHandle = this->device()->getVMA()->createImage(
		vk::ImageCreateInfo()
		.setImageType(this->mType)
		.setExtent(
			vk::Extent3D()
			.setWidth(this->mImageSize.x())
			.setHeight(this->mImageSize.y())
			.setDepth(this->mImageSize.z())
		)
		.setMipLevels(1)
		.setArrayLayers(1)
		.setFormat(this->mFormat)
		.setTiling(this->mTiling)
		.setUsage(this->mUsage)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(vk::SampleCountFlagBits::e1),
		this->mMemoryUsageFlag, this->mAllocated
	);
}

void* Image::get()
{
	return &this->mAllocated;
}

void Image::invalidate()
{
	if (this->mAllocHandle)
	{
		this->device()->getVMA()->destroyImage(this->mAllocated, this->mAllocHandle);
		this->mAllocHandle = nullptr;
	}
}

void Image::resetConfiguration()
{
}

uSize Image::getExpectedDataCount() const
{
	auto imgPixelCount = this->mImageSize.x() * this->mImageSize.y() * this->mImageSize.z();
	switch (this->mFormat)
	{
	case vk::Format::eR8G8B8A8Srgb: return imgPixelCount * 4;
	default:
		assert(false);
		return 0;
	}
}

void Image::transitionLayout(vk::ImageLayout prev, vk::ImageLayout next, CommandPool* transientPool)
{
	auto buffers = transientPool->createCommandBuffers(1);
	buffers[0]
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.setPipelineImageBarrier(this, prev, next)
		.end();
	auto queue = this->device()->getQueue(EQueueFamily::eGraphics);
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);
	/*
		TODO: This causes all commands to be synchronous.
		For higher throughput, this should not be called, and the command buffers should rely entirely
		on the pipeline barriers.
		This means multiple images could be sent to GPU at once.
	*/
	queue.waitIdle();
	buffers[0].destroy();
}

void Image::writeImage(void* data, uSize size, CommandPool* transientPool)
{
	auto const imgSize = this->getExpectedDataCount() * sizeof(ui8);
	assert(size <= imgSize);
	Buffer::writeDataToGPU(
		this->device(), transientPool,
		imgSize, /*bClear*/ false,
		/*offset*/ 0, data, size,
		[this, size](Command* cmd, Buffer *stagingBuffer) {
			cmd->copyBufferToImage(stagingBuffer, this);
		}
	);
}

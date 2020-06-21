#include "graphics/Image.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

Image& Image::setFormat(vk::Format format)
{
	this->mFormat = format;
	return *this;
}

Image& Image::setTiling(vk::ImageTiling tiling)
{
	this->mTiling = tiling;
	return *this;
}

Image& Image::setUsage(vk::ImageUsageFlags usage)
{
	this->mUsage = usage;
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

void Image::create(LogicalDevice *device)
{
	auto info = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
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
		.setSamples(vk::SampleCountFlagBits::e1);
	this->mInternal = device->mDevice->createImageUnique(info);

	this->createMemory(device, device->mDevice->getImageMemoryRequirements(this->mInternal.get()));
}

void* Image::get()
{
	return &this->mInternal.get();
}

void Image::invalidate()
{
	this->mInternal.reset();
	MemoryBacked::invalidate();
}

void Image::bind(LogicalDevice const *pDevice, vk::DeviceMemory &mem, uSize offset)
{
	pDevice->mDevice->bindImageMemory(this->mInternal.get(), mem, offset);
}

uSize Image::getExpectedDataSize() const
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

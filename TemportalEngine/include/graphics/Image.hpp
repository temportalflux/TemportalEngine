#pragma once

#include "graphics/MemoryBacked.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class Image : public MemoryBacked
{

public:
	Image() : mType(vk::ImageType::e2D) {}

	vk::ImageType getType() const { return this->mType; }
	Image& setFormat(vk::Format format);
	vk::Format getFormat() const;
	bool hasStencilComponent() const;
	Image& setTiling(vk::ImageTiling tiling);
	Image& setUsage(vk::ImageUsageFlags usage);
	Image& setSize(math::Vector3UInt const &size);
	math::Vector3UInt getSize() const;

	void create(LogicalDevice *device);
	void* get();
	void invalidate();

	uSize getExpectedDataSize() const;

private:
	vk::ImageType mType;
	vk::Format mFormat;
	vk::ImageTiling mTiling;
	vk::ImageUsageFlags mUsage;
	math::Vector3UInt mImageSize;
	vk::UniqueImage mInternal;

	void bind(LogicalDevice const *pDevice, vk::DeviceMemory &mem, uSize offset = 0) override;

};

NS_END

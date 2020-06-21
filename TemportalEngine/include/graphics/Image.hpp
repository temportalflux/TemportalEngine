#pragma once

#include "graphics/MemoryBacked.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class Image : public MemoryBacked
{

public:
	Image() = default;

	Image& setFormat(vk::Format format);
	Image& setTiling(vk::ImageTiling tiling);
	Image& setUsage(vk::ImageUsageFlags usage);
	Image& setSize(math::Vector3UInt const &size);
	math::Vector3UInt getSize() const;

	void create(LogicalDevice *device);
	void* get();
	void invalidate();

	uSize getExpectedDataSize() const;

private:
	vk::Format mFormat;
	vk::ImageTiling mTiling;
	vk::ImageUsageFlags mUsage;
	math::Vector3UInt mImageSize;
	vk::UniqueImage mInternal;

	void bind(LogicalDevice const *pDevice, vk::DeviceMemory &mem, uSize offset = 0) override;

};

NS_END

#pragma once

#include "graphics/MemoryBacked.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class PhysicalDevice;

class Buffer : public MemoryBacked
{

public:
	Buffer() = default;
	Buffer(Buffer &&other);
	Buffer& operator=(Buffer &&other);

	Buffer& setUsage(vk::BufferUsageFlags flags);
	Buffer& setSize(uSize size);
	uSize getSize() const;

	void create(LogicalDevice *pDevice);
	void destroy();

	void* get();

private:
	vk::BufferUsageFlags mUsageFlags;
	uSize mSize;
	vk::UniqueBuffer mInternal;

	void bind(LogicalDevice *pDevice, vk::DeviceMemory &mem, uSize offset = 0) override;

};

NS_END

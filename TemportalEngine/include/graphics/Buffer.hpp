#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <array>
#include <optional>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class PhysicalDevice;

class Buffer
{

public:
	Buffer() = default;
	Buffer(Buffer &&other);
	Buffer& operator=(Buffer &&other);

	Buffer& setUsage(vk::BufferUsageFlags flags);
	Buffer& setMemoryRequirements(vk::MemoryPropertyFlags flags);
	Buffer& setSize(uSize size);
	uSize getSize() const;

	void create(LogicalDevice const *pDevice);
	void destroy();

	void* get();

	void write(LogicalDevice const *pDevice, uSize offset, void* src, uSize size);

private:
	vk::BufferUsageFlags mUsageFlags;
	vk::MemoryPropertyFlags mMemoryFlags;
	uSize mSize;

	vk::UniqueBuffer mInternal;
	vk::UniqueDeviceMemory mBufferMemory;

	std::optional<ui32> findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags);

};

NS_END

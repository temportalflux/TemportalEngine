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
	Buffer& setSize(ui64 size);
	ui64 getSize() const;

	void create(LogicalDevice const *pDevice);
	void destroy();

	void* get();

	void write(LogicalDevice const *pDevice, ui64 offset, void* src, ui64 size);

private:
	vk::BufferUsageFlags mUsageFlags;
	vk::MemoryPropertyFlags mMemoryFlags;
	ui64 mSize;

	vk::UniqueBuffer mInternal;
	vk::UniqueDeviceMemory mBufferMemory;

	std::optional<ui32> findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags);

};

NS_END

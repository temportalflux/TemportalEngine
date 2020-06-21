#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class PhysicalDevice;

class MemoryBacked
{

public:
	MemoryBacked() = default;

	MemoryBacked& setMemoryRequirements(vk::MemoryPropertyFlags flags);

	uSize getMemorySize() const;

	void createMemory(LogicalDevice const *pDevice, vk::MemoryRequirements const &req);
	void invalidate();

	void write(LogicalDevice const *pDevice, uSize offset, void* src, uSize size);

protected:
	vk::MemoryPropertyFlags mMemoryFlags;
	vk::UniqueDeviceMemory mBufferMemory;
	uSize mMemorySize;

	std::optional<ui32> findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags);
	virtual void bind(LogicalDevice const *pDevice, vk::DeviceMemory &mem, uSize offset = 0) = 0;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamily.hpp"

#include <optional>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class PhysicalDevice;
class LogicalDeviceInfo;

class LogicalDevice
{
	friend class PhysicalDevice;

public:
	LogicalDevice() = default;

	std::optional<vk::Queue> getQueue(QueueFamily type) const;
	void invalidate();

private:
	PhysicalDevice const *mpPhysicalDevice;
	vk::UniqueDevice mDevice;

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

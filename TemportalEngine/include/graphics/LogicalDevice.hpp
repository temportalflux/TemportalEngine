#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamily.hpp"

#include <optional>
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <set>

NS_GRAPHICS
class PhysicalDevice;
class LogicalDeviceInfo;

class LogicalDevice
{
	friend class PhysicalDevice;

public:
	LogicalDevice() = default;

	std::unordered_map<QueueFamily, vk::Queue> findQueues(std::set<QueueFamily> types) const;
	void invalidate();

private:
	PhysicalDevice const *mpPhysicalDevice;
	vk::UniqueDevice mDevice;

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

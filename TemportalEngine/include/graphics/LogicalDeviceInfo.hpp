#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/real.h"
#include "types/integer.h"
#include "graphics/types.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class PhysicalDevice;
struct QueueFamilyGroup;

// TODO: Move to its own class
struct QueueInfo
{
	ui32 idxQueueFamily;
	std::vector<f32> queuePriorities;
	vk::DeviceQueueCreateInfo makeInfo() const;
};

class LogicalDeviceInfo
{
	friend class PhysicalDevice;

public:
	LogicalDeviceInfo() = default;

	LogicalDeviceInfo& addQueueFamily(QueueFamily::Enum type);
	LogicalDeviceInfo& addDeviceExtension(char const* name);
	LogicalDeviceInfo& setValidationLayers(std::vector<char const*> layers);

	std::set<QueueFamily::Enum> getQueues() const;

private:
	std::vector<QueueFamily::Enum> mQueues;
	std::vector<char const*> mDeviceExtensions;
	std::vector<char const*> mValidationLayers;

	std::vector<QueueInfo> makeQueueInfo(QueueFamilyGroup const *pAvailableQueues) const;

};

NS_END

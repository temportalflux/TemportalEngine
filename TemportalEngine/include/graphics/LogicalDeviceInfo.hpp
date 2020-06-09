#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"

// TODO: Move vulkan to only cpp. Only used by QueueInfo
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
	LogicalDeviceInfo& addDeviceExtension(std::string name);
	LogicalDeviceInfo& setValidationLayers(std::vector<std::string> layers);

	std::vector<std::string> getValidationLayers() const;

	std::set<QueueFamily::Enum> getQueues() const;

private:
	std::vector<QueueFamily::Enum> mQueues;
	std::vector<std::string> mDeviceExtensions;
	std::vector<std::string> mValidationLayers;

	std::vector<QueueInfo> makeQueueInfo(QueueFamilyGroup const *pAvailableQueues) const;

};

NS_END

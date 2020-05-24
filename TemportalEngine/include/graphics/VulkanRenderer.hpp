#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/QueueFamily.hpp"
#include "graphics/Surface.hpp"

#include <unordered_map>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevicePreference;

class VulkanRenderer
{

public:
	VulkanRenderer(VulkanInstance *pInstance, Surface &surface);

	void setPhysicalDevicePreference(PhysicalDevicePreference const &preference);
	void setLogicalDeviceInfo(LogicalDeviceInfo const &info);

	void initializeDevices();

	void invalidate();

private:
	VulkanInstance *mpInstance;
	Surface mSurface;

	PhysicalDevicePreference mPhysicalDevicePreference;
	PhysicalDevice mPhysicalDevice;
	
	LogicalDeviceInfo mLogicalDeviceInfo;
	LogicalDevice mLogicalDevice;

	std::unordered_map<QueueFamily, vk::Queue> mQueues;

	VulkanRenderer() = default;

	logging::Logger getLog() const;
	void pickPhysicalDevice();

};

NS_END

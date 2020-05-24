#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/QueueFamily.hpp"
#include "graphics/Surface.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/SwapChainInfo.hpp"

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
	void setSwapChainInfo(SwapChainInfo const &info);

	void initializeDevices();
	// Creates a swap chain, and all objects that depend on it
	void constructRenderChain();

	void invalidate();

private:
	VulkanInstance *mpInstance;
	Surface mSurface;

	PhysicalDevicePreference mPhysicalDevicePreference;
	PhysicalDevice mPhysicalDevice;
	
	LogicalDeviceInfo mLogicalDeviceInfo;
	LogicalDevice mLogicalDevice;
	std::unordered_map<QueueFamily, vk::Queue> mQueues;

	SwapChainInfo mSwapChainInfo;
	SwapChain mSwapChain;

	VulkanRenderer() = default;

	logging::Logger getLog() const;
	void pickPhysicalDevice();

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/SwapChainSupport.hpp"

#include <unordered_set>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class VulkanInstance;
class Surface;

class PhysicalDevice
{
	friend class VulkanInstance;

public:
	PhysicalDevice();
	PhysicalDevice(PhysicalDevice const &other);

	vk::PhysicalDeviceProperties const getProperties() const;
	std::unordered_set<std::string> getSupportedExtensionNames() const;
	vk::PhysicalDeviceFeatures const getFeatures() const;
	QueueFamilyGroup queryQueueFamilyGroup() const;
	SwapChainSupport querySwapChainSupport() const;

	void invalidate();

private:
	graphics::Surface *mpSurface;
	vk::PhysicalDevice mDevice;

	PhysicalDevice(vk::PhysicalDevice &device, graphics::Surface *const pSurface);
	vk::SurfaceKHR getVulkanSurface() const;

};

NS_END

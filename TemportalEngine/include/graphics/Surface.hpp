#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevice;
class SwapChain;

class Surface
{
	friend class VulkanInstance;
	friend class PhysicalDevice;
	friend class SwapChain;
	friend class VulkanApi;

public:
	Surface() = default;
	Surface(void* pWindowHandle);
	void releaseWindowHandle();
	
	void swap(Surface &other);

	// TODO: Replace return vec with engine-level structure
	vk::Extent2D getDrawableSize() const;

	Surface& initialize(VulkanInstance const *pVulkan);
	void destroy(VulkanInstance const *pVulkan);

private:
	void* mpWindowHandle;
	vk::UniqueSurfaceKHR mSurface;

};

NS_END
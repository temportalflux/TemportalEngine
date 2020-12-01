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

	void* getWindowHandle() const;
	// TODO: Replace return vec with engine-level structure
	vk::Extent2D getDrawableSize() const;

	Surface& initialize(std::shared_ptr<VulkanInstance> pVulkan);
	void* get();
	void destroy(std::shared_ptr<VulkanInstance> pVulkan);

private:
	void* mpWindowHandle;
	vk::UniqueSurfaceKHR mInternal;

};

NS_END
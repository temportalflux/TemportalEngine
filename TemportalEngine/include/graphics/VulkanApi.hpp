#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevice;
class LogicalDevice;
class RenderPass;

// Class stub for external hook-ins like ImGui which need access to low-level/C interfaces for vulkan.
// This class is friended to all Vulkan wrapper classes.
class VulkanApi
{

protected:

	VkInstance extract(VulkanInstance const *ptr) const;
	VkPhysicalDevice extract(PhysicalDevice const *ptr) const;
	VkDevice extract(LogicalDevice const *ptr) const;
	VkRenderPass extract(RenderPass const *ptr) const;

};

NS_END

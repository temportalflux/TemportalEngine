#include "graphics/VulkanApi.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/RenderPass.hpp"

using namespace graphics;

VkInstance VulkanApi::extract(VulkanInstance const *ptr) const { return (VkInstance)ptr->mInstance.get(); }
VkPhysicalDevice VulkanApi::extract(PhysicalDevice const *ptr) const { return (VkPhysicalDevice)ptr->mDevice; }
VkDevice VulkanApi::extract(LogicalDevice const *ptr) const { return (VkDevice)ptr->mDevice.get(); }
VkRenderPass VulkanApi::extract(RenderPass const *ptr) const { return (VkRenderPass)ptr->mRenderPass.get(); }

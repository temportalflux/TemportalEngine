#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamily.hpp"

#include <optional>
#include <unordered_map>
#include <set>
#include <vector>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class PhysicalDevice;
class LogicalDeviceInfo;

class LogicalDevice
{
	friend class PhysicalDevice;
	friend class SwapChain;
	friend class ShaderModule;
	friend class RenderPass;
	friend class Pipeline;
	friend class VulkanApi;

public:
	LogicalDevice() = default;

	bool isValid() const;
	void invalidate();

	std::unordered_map<QueueFamily, vk::Queue> findQueues(std::set<QueueFamily> types) const;

	vk::UniqueDevice mDevice; // TODO: Make private
private:
	PhysicalDevice const *mpPhysicalDevice;

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

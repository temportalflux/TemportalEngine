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

public:
	LogicalDevice() = default;

	bool isValid() const;
	void invalidate();

	std::unordered_map<QueueFamily, vk::Queue> findQueues(std::set<QueueFamily> types) const;

private:
	PhysicalDevice const *mpPhysicalDevice;
	vk::UniqueDevice mDevice;

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

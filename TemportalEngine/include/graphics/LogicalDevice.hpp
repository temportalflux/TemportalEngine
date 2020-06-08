#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"

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

	void* get();

	bool isValid() const;
	void invalidate();

	std::unordered_map<QueueFamily::Enum, vk::Queue> findQueues(std::set<QueueFamily::Enum> types) const;
	void waitUntilIdle() const;

	vk::UniqueDevice mDevice; // TODO: Make private
	PhysicalDevice const *mpPhysicalDevice; // TODO: Make Private
private:

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

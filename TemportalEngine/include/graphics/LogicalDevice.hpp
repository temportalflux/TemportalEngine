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
	friend class GraphicsDevice;

public:
	LogicalDevice() = default;

	void* get();

	bool isValid() const;
	void invalidate();

	std::unordered_map<QueueFamily::Enum, vk::Queue> findQueues(std::set<QueueFamily::Enum> types) const;
	void waitUntilIdle() const;
	void waitFor(std::vector<vk::Fence> fence, bool bAll, ui64 timeout);

	PhysicalDevice const *mpPhysicalDevice; // TODO: Make Private
private:
	vk::UniqueDevice mInternal;

	LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device);

};

NS_END

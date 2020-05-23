#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class PhysicalDevice;

class LogicalDevice
{
	friend class PhysicalDevice;

public:
	LogicalDevice() = default;

	void invalidate();

private:
	LogicalDevice(vk::UniqueDevice &device);

	vk::UniqueDevice mDevice;

};

NS_END

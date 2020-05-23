#pragma once

#include "TemportalEnginePCH.hpp"

#include <vector>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

struct SwapChainSupport
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
	std::vector<vk::PresentModeKHR> presentationModes;
};

NS_END

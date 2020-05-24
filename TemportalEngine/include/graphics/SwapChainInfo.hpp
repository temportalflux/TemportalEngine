#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/SwapChainSupport.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

// TODO: Expose adjustments to variables for user to set preferences
class SwapChainInfo
{

public:
	vk::Extent2D getAdjustedResolution(vk::Extent2D drawableSize, vk::SurfaceCapabilitiesKHR const &capabilities) const;
	vk::SurfaceFormatKHR selectSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) const;
	vk::PresentModeKHR selectPresentationMode(std::vector<vk::PresentModeKHR> const &availableModes) const;

};

NS_END

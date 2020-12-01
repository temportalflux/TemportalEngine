#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/SwapChainSupport.hpp"

#include <set>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

// TODO: Expose adjustments to variables for user to set preferences
class SwapChainInfo
{

public:
	SwapChainInfo& addFormatPreference(vk::Format const &format);
	SwapChainInfo& setColorSpace(vk::ColorSpaceKHR const &colorSpace);
	SwapChainInfo& addPresentModePreference(vk::PresentModeKHR const &mode);

	vk::Extent2D getAdjustedResolution(vk::Extent2D drawableSize, vk::SurfaceCapabilitiesKHR const &capabilities) const;
	vk::SurfaceFormatKHR selectSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) const;
	vk::PresentModeKHR selectPresentationMode(std::vector<vk::PresentModeKHR> const &availableModes) const;

private:
	// an ordered set of formats. the most prefered should be first, least prefered last.
	std::vector<vk::Format> mFormatPreferences;
	vk::ColorSpaceKHR mColorSpace;
	// an ordered set of present modes. the most prefered should be first, least prefered last.
	std::vector<vk::PresentModeKHR> mPresentModePreferences;

};

NS_END

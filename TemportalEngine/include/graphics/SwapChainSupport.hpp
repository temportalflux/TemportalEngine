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

	SwapChainSupport(vk::PhysicalDevice const &device, vk::SurfaceKHR const &surface)
	{
		capabilities = device.getSurfaceCapabilitiesKHR(surface);
		surfaceFormats = device.getSurfaceFormatsKHR(surface);
		presentationModes = device.getSurfacePresentModesKHR(surface);
	}

};

NS_END

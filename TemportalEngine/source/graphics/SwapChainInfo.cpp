#include "graphics/SwapChainInfo.hpp"

using namespace graphics;

vk::Extent2D SwapChainInfo::getAdjustedResolution(vk::Extent2D drawableSize, vk::SurfaceCapabilitiesKHR const &capabilities) const
{
	if (capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;
	return vk::Extent2D()
		.setWidth(
			max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, drawableSize.width))
		)
		.setHeight(
			max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, drawableSize.height))
		);
}


vk::SurfaceFormatKHR SwapChainInfo::selectSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) const
{
	for (const auto& format : availableFormats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}
	return availableFormats[0];
}

vk::PresentModeKHR SwapChainInfo::selectPresentationMode(std::vector<vk::PresentModeKHR> const &availableModes) const
{
	for (const auto& mode : availableModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	return vk::PresentModeKHR::eFifo;
}

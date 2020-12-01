#include "graphics/SwapChainInfo.hpp"

#include "types/integer.h"
#include "math/values.hpp"

#include <map>
#include <unordered_set>

using namespace graphics;

SwapChainInfo& SwapChainInfo::addFormatPreference(vk::Format const &format)
{
	this->mFormatPreferences.push_back(format);
	return *this;
}

SwapChainInfo& SwapChainInfo::setColorSpace(vk::ColorSpaceKHR const &colorSpace)
{
	this->mColorSpace = colorSpace;
	return *this;
}

SwapChainInfo& SwapChainInfo::addPresentModePreference(vk::PresentModeKHR const &mode)
{
	this->mPresentModePreferences.push_back(mode);
	return *this;
}

vk::Extent2D SwapChainInfo::getAdjustedResolution(vk::Extent2D drawableSize, vk::SurfaceCapabilitiesKHR const &capabilities) const
{
	if (capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;
	return vk::Extent2D()
		.setWidth(
			math::max(capabilities.minImageExtent.width, math::min(capabilities.maxImageExtent.width, drawableSize.width))
		)
		.setHeight(
			math::max(capabilities.minImageExtent.height, math::min(capabilities.maxImageExtent.height, drawableSize.height))
		);
}

vk::SurfaceFormatKHR SwapChainInfo::selectSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) const
{
	auto prefCount = this->mFormatPreferences.size();
	auto candidates = std::multimap<uSize, vk::SurfaceFormatKHR>();
	for (const auto& format : availableFormats)
	{
		if (format.colorSpace != this->mColorSpace) continue;
		for (uSize i = 0; i < prefCount; ++i)
		{
			if (format.format == this->mFormatPreferences[i])
			{
				candidates.insert(std::make_pair(i, format));
				break;
			}
		}
	}
	return candidates.size() > 0 ? candidates.cbegin()->second : availableFormats[0];
}

vk::PresentModeKHR SwapChainInfo::selectPresentationMode(std::vector<vk::PresentModeKHR> const &availableModes) const
{
	auto availableModeSet = std::unordered_set<vk::PresentModeKHR>(availableModes.begin(), availableModes.end());
	for (const auto& pref : this->mPresentModePreferences)
	{
		if (availableModeSet.find(pref) != availableModeSet.end()) return pref;
	}
	return vk::PresentModeKHR::eFifo;
}

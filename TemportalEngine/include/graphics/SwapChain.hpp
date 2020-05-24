#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class Surface;

class SwapChain
{

public:
	SwapChain();

	SwapChain& setInfo(SwapChainInfo const &info);
	SwapChain& setSupport(SwapChainSupport const &support);
	SwapChain& setQueueFamilyGroup(QueueFamilyGroup const &qfg);

	void create(LogicalDevice const *pDevice, Surface const *pSurface);
	void destroy();

private:
	SwapChainInfo mInfo;
	SwapChainSupport mSupport;
	QueueFamilyGroup mQueueFamilyGroup;

	LogicalDevice const *mpDevice;

	vk::Extent2D mDrawableSize;
	vk::Extent2D mResolution;
	vk::SurfaceFormatKHR mSurfaceFormat;
	vk::PresentModeKHR mPresentationMode;

	vk::UniqueSwapchainKHR mSwapChain;

	std::vector<vk::Image> queryImages() const;
	std::vector<vk::UniqueImageView> createImageViews() const;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "graphics/ImageViewInfo.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class Surface;

class SwapChain
{
	friend class RenderPass;

public:
	SwapChain();

	SwapChain& setInfo(SwapChainInfo const &info);
	SwapChain& setSupport(SwapChainSupport const &support);
	SwapChain& setQueueFamilyGroup(QueueFamilyGroup const &qfg);

	SwapChain& create(LogicalDevice const *pDevice, Surface const *pSurface);
	void destroy();

	std::vector<vk::UniqueImageView> createImageViews(ImageViewInfo const &info) const;
	vk::Extent2D getResolution() const;

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

};

NS_END

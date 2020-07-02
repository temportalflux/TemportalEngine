#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "graphics/ImageViewInfo.hpp"
#include "graphics/ImageView.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class Surface;

class SwapChain
{
	friend class VulkanApi;
	friend class RenderPass;
	friend class Frame;

public:
	SwapChain();

	SwapChain& setInfo(SwapChainInfo const &info);
	SwapChain& setSupport(SwapChainSupport const &support);
	SwapChain& setQueueFamilyGroup(QueueFamilyGroup const &qfg);

	SwapChain& create(LogicalDevice *pDevice, Surface const *pSurface);
	void destroy();

	std::vector<ImageView> createImageViews(ImageViewInfo const &info) const;
	ui32 getFormat() const;
	math::Vector2UInt getResolution() const;

	vk::ResultValue<ui32> acquireNextImage(
		std::optional<vk::Semaphore> waitSemaphore = std::nullopt,
		std::optional<vk::Fence> waitFence = std::nullopt
	) const;
	vk::SwapchainKHR get() const;

private:
	SwapChainInfo mInfo;
	SwapChainSupport mSupport;
	QueueFamilyGroup mQueueFamilyGroup;

	LogicalDevice *mpDevice;

	vk::Extent2D mDrawableSize;
	math::Vector2UInt mResolution;
	vk::SurfaceFormatKHR mSurfaceFormat;
	vk::PresentModeKHR mPresentationMode;

	vk::UniqueSwapchainKHR mInternal;

	std::vector<vk::Image> queryImages() const;

};

NS_END

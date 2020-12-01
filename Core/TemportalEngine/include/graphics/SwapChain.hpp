#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "graphics/ImageViewInfo.hpp"
#include "graphics/ImageView.hpp"
#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class SwapChain : public DeviceObject
{
	friend class GraphicsDevice;

	friend class VulkanApi;
	friend class RenderPass;
	friend class Frame;

public:
	SwapChain();

	SwapChain& setInfo(SwapChainInfo const &info);
	SwapChain& setSupport(SwapChainSupport const &support);
	SwapChain& setQueueFamilyGroup(QueueFamilyGroup const &qfg);

	SwapChain& setSurface(class Surface *pSurface);

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

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

	vk::Extent2D mDrawableSize;
	vk::SurfaceKHR mSurface;
	math::Vector2UInt mResolution;
	vk::SurfaceFormatKHR mSurfaceFormat;
	vk::PresentModeKHR mPresentationMode;

	vk::UniqueSwapchainKHR mInternal;

};

NS_END

#include "graphics/SwapChain.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/Surface.hpp"
#include "graphics/VulkanApi.hpp"
#include "types/math.h"

using namespace graphics;

SwapChain::SwapChain()
{
}

SwapChain& SwapChain::setInfo(SwapChainInfo const &info)
{
	this->mInfo = info;
	return *this;
}

SwapChain& SwapChain::setSupport(SwapChainSupport const &support)
{
	this->mSupport = support;
	return *this;
}

SwapChain& SwapChain::setQueueFamilyGroup(QueueFamilyGroup const &qfg)
{
	this->mQueueFamilyGroup = qfg;
	return *this;
}

SwapChain& SwapChain::setSurface(Surface *pSurface)
{
	this->mDrawableSize = pSurface->getDrawableSize();
	this->mSurface = extract<vk::SurfaceKHR>(pSurface);
	return *this;
}

void SwapChain::create()
{
	auto resolution = mInfo.getAdjustedResolution(mDrawableSize, mSupport.capabilities);
	this->mResolution = { resolution.width, resolution.height };
	mSurfaceFormat = mInfo.selectSurfaceFormat(mSupport.surfaceFormats);
	mPresentationMode = mInfo.selectPresentationMode(mSupport.presentationModes);
	// Adding at least 1 more to the chain will help avoid waiting on the GPU to complete internal ops before showing the next buffer.
	// Ensure that the image count does not exceed the max, unless the max is == 0
	ui32 minImageCount = minUnless(mSupport.capabilities.minImageCount + 1, mSupport.capabilities.maxImageCount, 0);

	auto info = vk::SwapchainCreateInfoKHR()
		// Hard set internally
		.setSurface(this->mSurface)
		.setPreTransform(mSupport.capabilities.currentTransform) // IMGUI by default was using VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
		.setOldSwapchain(nullptr) // swap chain must be recreated if the window is resized. this is not yet handled
		// Determined above
		.setImageExtent(resolution)
		.setImageFormat(mSurfaceFormat.format)
		.setImageColorSpace(mSurfaceFormat.colorSpace)
		.setPresentMode(mPresentationMode)
		.setMinImageCount(minImageCount)
		// TODO: These variables can be driven by SwapChainInfo
		.setImageArrayLayers(1) // always 1 unless developing a stereoscopic 3D application
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setClipped(true);

	// TODO: Assumes there is always a graphics and presentation queue. Can the information about what queues are actually in use go in QFG?
	auto allQueues = mQueueFamilyGroup.allQueues(); // need to retain in memory until after the vulkan device is deleted (setPQueueFamilyIndices doesnt copy the data)
	if (mQueueFamilyGroup.idxGraphicsQueue != mQueueFamilyGroup.idxPresentationQueue)
	{
		info.setImageSharingMode(vk::SharingMode::eConcurrent);
		info.setQueueFamilyIndexCount(2);
		info.setPQueueFamilyIndices(allQueues.data());
	}
	else
	{
		info.setImageSharingMode(vk::SharingMode::eExclusive);
		info.setQueueFamilyIndexCount(0);
		info.setPQueueFamilyIndices(nullptr);
	}

	this->mInternal = this->device()->createSwapchain(info);
}

void* SwapChain::get()
{
	return &this->mInternal.get();
}

void SwapChain::invalidate()
{
	this->mInternal.reset();
}

void SwapChain::resetConfiguration()
{

}

ui32 SwapChain::getFormat() const
{
	return (ui32)this->mSurfaceFormat.format;
}

math::Vector2UInt SwapChain::getResolution() const
{
	return mResolution;
}

std::vector<ImageView> SwapChain::createImageViews(ImageViewInfo const &info) const
{
	auto images = this->device()->getSwapChainImages(this);
	uSize imageCount = images.size();
	auto views = std::vector<ImageView>(imageCount);
	for (uSize i = 0; i < imageCount; ++i)
	{
		views[i].setDevice(this->device());
		// Setup constant properties on each view
		views[i]
			.setFormat(this->mSurfaceFormat.format)
			.setViewType(info.type).setComponentMapping(info.swizzle)
			// TODO: These can be configured by something in the future
			.setRange(
				vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0).setLevelCount(1)
				.setBaseArrayLayer(0).setLayerCount(1)
			);
		// Set the image on the view that has actually changed
		views[i].setRawImage(images[i]);
		// Construct the view for the provided data
		views[i].create();
	}
	return views;
}

vk::ResultValue<ui32> SwapChain::acquireNextImage(
	std::optional<vk::Semaphore> waitSemaphore,
	std::optional<vk::Fence> waitFence
) const
{
	return this->device()->acquireNextImageIndex(this, UINT64_MAX, waitSemaphore, waitFence);
}

vk::SwapchainKHR SwapChain::get() const
{
	return this->mInternal.get();
}

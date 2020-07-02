#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer()
	: mIdxCurrentFrame(0)
	, mbRenderChainDirty(false)
{
}

void VulkanRenderer::setInstance(VulkanInstance *pInstance)
{
	this->mpInstance = pInstance;
}

void VulkanRenderer::takeOwnershipOfSurface(Surface &surface)
{
	this->mSurface.swap(surface);
}

logging::Logger VulkanRenderer::getLog() const
{
	return mpInstance->getLog();
}

void VulkanRenderer::setPhysicalDevicePreference(PhysicalDevicePreference const &preference)
{
	mPhysicalDevicePreference = preference;
}

void VulkanRenderer::setLogicalDeviceInfo(LogicalDeviceInfo const &info)
{
	auto layers = this->mLogicalDeviceInfo.getValidationLayers();
	mLogicalDeviceInfo = info;
	this->setValidationLayers(layers);
}

void VulkanRenderer::setValidationLayers(std::vector<std::string> layers)
{
	this->mLogicalDeviceInfo.setValidationLayers(layers);
}

void VulkanRenderer::setSwapChainInfo(SwapChainInfo const &info)
{
	mSwapChainInfo = info;
}

void VulkanRenderer::setImageViewInfo(ImageViewInfo const &info)
{
	mImageViewInfo = info;
}

f32 VulkanRenderer::getAspectRatio() const
{
	auto resolution = this->mSwapChain.getResolution();
	return resolution.x() / (f32)resolution.y();
}

void VulkanRenderer::initializeDevices()
{
	this->pickPhysicalDevice();
	this->mLogicalDevice = this->mPhysicalDevice.createLogicalDevice(&mLogicalDeviceInfo, &mPhysicalDevicePreference);
	this->mQueues = this->mLogicalDevice.findQueues(mLogicalDeviceInfo.getQueues());
}

vk::Queue& VulkanRenderer::getQueue(QueueFamily::Enum type)
{
	return this->mQueues[type];
}

void VulkanRenderer::pickPhysicalDevice()
{
	auto optPhysicalDevice = mpInstance->pickPhysicalDevice(mPhysicalDevicePreference, &mSurface);
	if (!optPhysicalDevice.has_value())
	{
		getLog().log(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device.");
		return;
	}
	mPhysicalDevice = optPhysicalDevice.value();
}

void VulkanRenderer::invalidate()
{
	this->mQueues.clear();
	this->mLogicalDevice.invalidate();
	this->mPhysicalDevice.invalidate();

	mSurface.destroy(mpInstance);
	mSurface.releaseWindowHandle();

	mpInstance = nullptr;
}

void VulkanRenderer::createSwapChain()
{
	this->mSwapChain
		.setInfo(mSwapChainInfo)
		.setSupport(mPhysicalDevice.querySwapChainSupport())
		.setQueueFamilyGroup(mPhysicalDevice.queryQueueFamilyGroup())
		.create(&mLogicalDevice, &mSurface);
}

void VulkanRenderer::createFrameImageViews()
{
	auto views = this->mSwapChain.createImageViews(this->mImageViewInfo);
	auto viewCount = views.size();
	this->mFrameImageViews.resize(viewCount);
	this->mFrameImageFences.resize(viewCount);
	for (uSize i = 0; i < viewCount; ++i)
		this->mFrameImageViews[i] = std::move(views[i]);
}

void VulkanRenderer::createRenderPass(std::optional<vk::Format> depthBufferFormat)
{
	this->mRenderPass
		.setFormat(this->mSwapChain.getFormat())
		.setScissorBounds({ 0, 0 }, this->mSwapChain.getResolution())
		.create(&this->mLogicalDevice, depthBufferFormat);
}

void VulkanRenderer::destroySwapChain()
{
	this->mSwapChain.destroy();
}

void VulkanRenderer::destroyFrameImageViews()
{
	this->mFrameImageViews.clear();
	this->mFrameImageFences.clear();
}

void VulkanRenderer::destroyRenderPass()
{
	this->mRenderPass.destroy();
}

void VulkanRenderer::drawFrame()
{
	if (this->mbRenderChainDirty) return;
	
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	currentFrame->waitUntilNotInFlight();
	
	if (!this->acquireNextImage()) return;
	this->prepareRender((ui32)this->mIdxCurrentFrame);
	this->render(currentFrame, this->mIdxCurrentImage);
	if (!this->present()) return;
	
	this->mIdxCurrentFrame = (this->mIdxCurrentFrame + 1) % this->getNumberOfFrames();
}

void VulkanRenderer::prepareRender(ui32 idxCurrentFrame)
{
	auto* currentFrame = this->getFrameAt(idxCurrentFrame);

	// If the next image view is currently in flight, wait until it isn't (it is being used by another frame)
	auto& frameImageFence = this->mFrameImageFences[this->mIdxCurrentImage];
	if (frameImageFence)
	{
		this->mLogicalDevice.mDevice->waitForFences(frameImageFence, true, UINT64_MAX);
	}

	// Ensure that the image view is marked as in-flight as long as the frame is
	frameImageFence = currentFrame->getInFlightFence();

	// Mark frame and image view as not in flight (will be in flight when queue is submitted)
	currentFrame->markNotInFlight();
}

bool VulkanRenderer::acquireNextImage()
{
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);

	try
	{
		auto [result, idxImage] = currentFrame->acquireNextImage(&this->mSwapChain);
		this->mIdxCurrentImage = idxImage;
	}
	catch (vk::OutOfDateKHRError const &e)
	{
		(void)e; // removes unreferenced compiler exception
		// Only mark as dirty if it is not already dirty.
		if (!this->mbRenderChainDirty) this->markRenderChainDirty();
		// Once dirty has been detected, stall until it is resolved
		return false;
	}
	return true;
}

bool VulkanRenderer::present()
{
	if (this->mbRenderChainDirty) return false;

	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	try
	{
		auto result = currentFrame->present(&this->mQueues[QueueFamily::Enum::ePresentation], { &mSwapChain }, mIdxCurrentImage);
		if (result == vk::Result::eSuboptimalKHR)
		{
			if (!this->mbRenderChainDirty) this->markRenderChainDirty();
			return false;
		}
	}
	catch (vk::OutOfDateKHRError const &e)
	{
		(void)e; // removes unreferenced compiler exception
		// Only mark as dirty if it is not already dirty.
		if (!this->mbRenderChainDirty) this->markRenderChainDirty();
		// Once dirty has been detected, stall until it is resolved.
		return false;
	}
	return true;
}

void VulkanRenderer::waitUntilIdle()
{
	this->mLogicalDevice.waitUntilIdle();
}

void VulkanRenderer::markRenderChainDirty()
{
	this->mbRenderChainDirty = true;
}

void VulkanRenderer::update()
{
	if (this->mbRenderChainDirty)
	{
		this->waitUntilIdle();
		this->destroyRenderChain();

		// Stall creating the render chain until the window is not minimized
		auto actualSize = mPhysicalDevice.querySwapChainSupport().capabilities.currentExtent;
		if (actualSize.width <= 0 || actualSize.height <= 0) return;

		this->createRenderChain();
		this->mIdxCurrentFrame = 0;
		this->mIdxCurrentImage = 0;
		this->mbRenderChainDirty = false;
	}
}

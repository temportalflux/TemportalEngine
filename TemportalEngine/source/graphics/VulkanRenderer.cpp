#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer()
	: mIdxCurrentFrame(0)
	, mbRenderChainDirty(false)
{
}

void VulkanRenderer::setInstance(std::shared_ptr<VulkanInstance> pInstance)
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
	this->mpGraphicsDevice = std::make_shared<GraphicsDevice>(this->mpInstance);
	this->mpGraphicsDevice->create(this->mPhysicalDevicePreference, this->mLogicalDeviceInfo, &this->mSurface);
}

vk::Queue const& VulkanRenderer::getQueue(QueueFamily::Enum type) const
{
	return this->mpGraphicsDevice->getQueue(type);
}

void VulkanRenderer::invalidate()
{
	assert(this->mpGraphicsDevice.use_count() == 1);
	this->mpGraphicsDevice->destroy();
	this->mpGraphicsDevice.reset();

	mSurface.destroy(mpInstance);
	mSurface.releaseWindowHandle();

	mpInstance = nullptr;
}

void VulkanRenderer::createSwapChain()
{
	this->mSwapChain
		.setInfo(mSwapChainInfo)
		.setSupport(this->mpGraphicsDevice->querySwapChainSupport())
		.setQueueFamilyGroup(this->mpGraphicsDevice->queryQueueFamilyGroup())
		.create(this->mpGraphicsDevice, &mSurface);
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

void VulkanRenderer::destroySwapChain()
{
	this->mSwapChain.destroy();
}

void VulkanRenderer::destroyFrameImageViews()
{
	this->mFrameImageViews.clear();
	this->mFrameImageFences.clear();
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
	
	this->onFramePresented(this->mIdxCurrentFrame);
	this->mIdxCurrentFrame = (this->mIdxCurrentFrame + 1) % this->getNumberOfFrames();
}

void VulkanRenderer::prepareRender(ui32 idxCurrentFrame)
{
	auto* currentFrame = this->getFrameAt(idxCurrentFrame);

	// If the next image view is currently in flight, wait until it isn't (it is being used by another frame)
	auto& frameImageFence = this->mFrameImageFences[this->mIdxCurrentImage];
	if (frameImageFence)
	{
		this->mpGraphicsDevice->logical().waitFor({ frameImageFence }, true, UINT64_MAX);
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
		auto result = currentFrame->present(&this->getQueue(QueueFamily::Enum::ePresentation), { &mSwapChain }, mIdxCurrentImage);
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
	this->mpGraphicsDevice->logical().waitUntilIdle();
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
		auto actualSize = this->mpGraphicsDevice->querySwapChainSupport().capabilities.currentExtent;
		if (actualSize.width <= 0 || actualSize.height <= 0) return;

		this->createRenderChain();
		this->mIdxCurrentFrame = 0;
		this->mIdxCurrentImage = 0;
		this->mbRenderChainDirty = false;
	}
}

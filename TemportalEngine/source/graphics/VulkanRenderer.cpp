#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer(VulkanInstance *pInstance, Surface &surface)
	: mpInstance(pInstance)
	, mIdxCurrentFrame(0)
	, mbRenderChainDirty(false)
{
	mSurface.swap(surface);
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		);
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
	mLogicalDeviceInfo = info;
}

void VulkanRenderer::setSwapChainInfo(SwapChainInfo const &info)
{
	mSwapChainInfo = info;
}

void VulkanRenderer::setImageViewInfo(ImageViewInfo const &info)
{
	mImageViewInfo = info;
}

void VulkanRenderer::addShader(std::shared_ptr<ShaderModule> shader)
{
	this->mPipeline.addShader(shader);
}

void VulkanRenderer::initializeDevices()
{
	this->pickPhysicalDevice();
	this->mLogicalDevice = this->mPhysicalDevice.createLogicalDevice(&mLogicalDeviceInfo);
	this->mQueues = this->mLogicalDevice.findQueues(mLogicalDeviceInfo.getQueues());
}

vk::Queue& VulkanRenderer::getQueue(QueueFamily type)
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
	this->destroyRenderChain();

	this->destroyInputBuffers();

	this->mQueues.clear();
	this->mLogicalDevice.invalidate();
	this->mPhysicalDevice.invalidate();

	mSurface.destroy(mpInstance);
	mSurface.releaseWindowHandle();

	mpInstance = nullptr;
}

void VulkanRenderer::createRenderChain()
{
	this->createRenderObjects();
	this->createCommandObjects();
	this->createFrames(this->mImageViews.size());
}

void VulkanRenderer::destroyRenderChain()
{
	this->destroyFrames();
	this->destroyCommandObjects();
	this->destroyRenderObjects();
}

void VulkanRenderer::createRenderObjects()
{
	this->mSwapChain
		.setInfo(mSwapChainInfo)
		.setSupport(mPhysicalDevice.querySwapChainSupport())
		.setQueueFamilyGroup(mPhysicalDevice.queryQueueFamilyGroup())
		.create(&mLogicalDevice, &mSurface);

	this->mImageViews = this->mSwapChain.createImageViews(mImageViewInfo);

	this->mRenderPass.initFromSwapChain(&this->mSwapChain).create(&this->mLogicalDevice);
}

void VulkanRenderer::destroyRenderObjects()
{
	this->mRenderPass.destroy();
	this->mImageViews.clear();
	this->mSwapChain.destroy();
}

void VulkanRenderer::createCommandObjects()
{
	auto resolution = this->mSwapChain.getResolution();

	this->mFrameBuffers = this->mRenderPass.createFrameBuffers(this->mImageViews);
	
	this->mPipeline.setViewArea(
		vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.width).setHeight((f32)resolution.height)
		.setMinDepth(0.0f).setMaxDepth(1.0f),
		vk::Rect2D().setOffset({ 0, 0 }).setExtent(resolution)
	);
	this->mPipeline.create(&this->mLogicalDevice, &this->mRenderPass);
	
	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mImageViews.size());
	
	this->recordCommandBufferInstructions();
}

void VulkanRenderer::destroyCommandObjects()
{
	this->mCommandBuffers.clear();
	this->mCommandPool.destroy();
	this->mPipeline.destroy();
	this->mFrameBuffers.clear();
}

void VulkanRenderer::createInputBuffers(ui32 bufferSize)
{
	auto vertexShader = this->mPipeline.getShader(vk::ShaderStageFlagBits::eVertex);
	assert(vertexShader != nullptr);
	this->mVertexBuffer.setSize(bufferSize).create(&this->mLogicalDevice);
}

void VulkanRenderer::destroyInputBuffers()
{
	this->mVertexBuffer.destroy();
}

void VulkanRenderer::createFrames(uSize viewCount)
{
	this->mFrames.resize(viewCount);
	for (auto& frame : this->mFrames)
	{
		frame.create(&this->mLogicalDevice);
	}
}

uSize VulkanRenderer::getNumberOfFrames() const
{
	return this->mFrames.size();
}

graphics::Frame* VulkanRenderer::getFrameAt(uSize idx)
{
	return &this->mFrames[idx];
}

void VulkanRenderer::destroyFrames()
{
	this->mFrames.clear();
}

void VulkanRenderer::recordCommandBufferInstructions()
{
	for (uSize i = 0; i < this->mCommandBuffers.size(); ++i)
	{
		this->mCommandBuffers[i].beginCommand()
			.clear({ 0.0f, 0.0f, 0.0f, 1.0f })
			.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[i])
			.bindPipeline(&this->mPipeline)
			.bindVertexBuffers({ &this->mVertexBuffer })
			.draw(this->mVertexCount)
			.endRenderPass()
			.end();
	}
}

void VulkanRenderer::drawFrame()
{
	if (this->mbRenderChainDirty) return;
	
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	currentFrame->waitUntilNotInFlight();
	
	if (!this->acquireNextImage()) return;
	this->prepareRender();
	this->render();
	if (!this->present()) return;
	
	this->mIdxCurrentFrame = (this->mIdxCurrentFrame + 1) % this->getNumberOfFrames();
}

void VulkanRenderer::prepareRender()
{
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);

	// If the next image view is currently in flight, wait until it isn't (it is being used by another frame)
	auto& imageView = this->mImageViews[mIdxCurrentImage];
	if (imageView.isInFlight())
	{
		imageView.waitUntilNotInFlight(&this->mLogicalDevice);
	}

	// Ensure that the image view is marked as in-flight as long as the frame is
	currentFrame->setImageViewInFlight(&imageView);

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

void VulkanRenderer::render()
{
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mCommandBuffers[this->mIdxCurrentImage];
	currentFrame->submitBuffers(&this->mQueues[QueueFamily::eGraphics], { &commandBuffer });
}

bool VulkanRenderer::present()
{
	if (this->mbRenderChainDirty) return false;

	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	try
	{
		auto result = currentFrame->present(&this->mQueues[QueueFamily::ePresentation], { &mSwapChain }, mIdxCurrentImage);
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

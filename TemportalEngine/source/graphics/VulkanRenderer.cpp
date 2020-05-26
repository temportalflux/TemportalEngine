#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer(VulkanInstance *pInstance, Surface &surface)
	: mpInstance(pInstance)
	, mIdxCurrentFrame(0)
{
	mSurface.swap(surface);
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

void VulkanRenderer::initializeDevices()
{
	this->pickPhysicalDevice();
	this->mLogicalDevice = this->mPhysicalDevice.createLogicalDevice(&mLogicalDeviceInfo);
	this->mQueues = this->mLogicalDevice.findQueues(mLogicalDeviceInfo.getQueues());
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

void VulkanRenderer::constructRenderChain(std::set<ShaderModule*> const &shaders)
{
	this->mSwapChain
		.setInfo(mSwapChainInfo)
		.setSupport(mPhysicalDevice.querySwapChainSupport())
		.setQueueFamilyGroup(mPhysicalDevice.queryQueueFamilyGroup())
		.create(&mLogicalDevice, &mSurface);

	this->mImageViews = this->mSwapChain.createImageViews({
		vk::ImageViewType::e2D,
		{
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		}
	});

	auto resolution = this->mSwapChain.getResolution();
	this->mRenderPass.initFromSwapChain(&this->mSwapChain).create(&this->mLogicalDevice);

	this->mFrameBuffers = this->mRenderPass.createFrameBuffers(this->mImageViews);
	
	this->mPipeline.setViewArea(
		vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.width).setHeight((f32)resolution.height)
		.setMinDepth(0.0f).setMaxDepth(1.0f),
		vk::Rect2D().setOffset({ 0, 0 }).setExtent(resolution)
	);
	for (auto& shaderPtr : shaders)
	{
		this->mPipeline.addShader(shaderPtr);
	}
	this->mPipeline.create(&this->mLogicalDevice, &this->mRenderPass);
	
	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mImageViews.size());
	
	this->recordCommandBufferInstructions();

	// TODO: make the number of frames not a magic number
	this->mFrames.resize(/*max number of frames*/ 3);
	for (auto& frame : this->mFrames)
	{
		frame.create(&this->mLogicalDevice);
	}
}

void VulkanRenderer::recordCommandBufferInstructions()
{
	for (uSize i = 0; i < this->mCommandBuffers.size(); ++i)
	{
		this->mCommandBuffers[i].beginCommand()
			.clear({ 0.0f, 0.0f, 0.0f, 1.0f })
			.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[i])
			.bindPipeline(&this->mPipeline)
			.draw()
			.endRenderPass()
			.end();
	}
}

void VulkanRenderer::invalidate()
{
	this->mFrames.clear();

	this->mCommandBuffers.clear();
	this->mCommandPool.destroy();
	this->mPipeline.destroy();
	this->mFrameBuffers.clear();
	this->mRenderPass.destroy();
	this->mImageViews.clear();
	this->mSwapChain.destroy();

	this->mQueues.clear();
	this->mLogicalDevice.invalidate();
	this->mPhysicalDevice.invalidate();

	mSurface.destroy(mpInstance);
	mSurface.releaseWindowHandle();

	mpInstance = nullptr;
}

void VulkanRenderer::drawFrame()
{
	auto& currentFrame = this->mFrames[this->mIdxCurrentFrame];
	currentFrame.waitUntilNotInFlight();
	if (!this->acquireNextImage()) return;
	this->prepareRender();
	this->render();
	this->present();
	this->mIdxCurrentFrame = (this->mIdxCurrentFrame + 1) % this->mFrames.size();
}

void VulkanRenderer::prepareRender()
{
	auto& currentFrame = this->mFrames[this->mIdxCurrentFrame];

	// If the next image view is currently in flight, wait until it isn't (it is being used by another frame)
	auto& imageView = this->mImageViews[mIdxCurrentImage];
	if (imageView.isInFlight())
	{
		imageView.waitUntilNotInFlight(&this->mLogicalDevice);
	}

	// Ensure that the image view is marked as in-flight as long as the frame is
	currentFrame.setImageViewInFlight(&imageView);

	// Mark frame and image view as not in flight (will be in flight when queue is submitted)
	currentFrame.markNotInFlight();
}

bool VulkanRenderer::acquireNextImage()
{
	auto& currentFrame = this->mFrames[this->mIdxCurrentFrame];
	auto [result, idxImage] = currentFrame.acquireNextImage(&this->mSwapChain);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{

		return false;
	}

	mIdxCurrentImage = idxImage;
	return true;
}

void VulkanRenderer::render()
{
	auto& currentFrame = this->mFrames[this->mIdxCurrentFrame];
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mCommandBuffers[mIdxCurrentImage];
	currentFrame.submitBuffers(&this->mQueues[QueueFamily::eGraphics], { &commandBuffer });
}

void VulkanRenderer::present()
{
	auto& currentFrame = this->mFrames[this->mIdxCurrentFrame];
	currentFrame.present(&this->mQueues[QueueFamily::ePresentation], { &mSwapChain }, mIdxCurrentImage);
}

void VulkanRenderer::waitUntilIdle()
{
	this->mLogicalDevice.waitUntilIdle();
}

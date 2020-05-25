#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer(VulkanInstance *pInstance, Surface &surface)
	: mpInstance(pInstance)
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

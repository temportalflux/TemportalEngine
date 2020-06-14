#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/Uniform.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer()
	: mIdxCurrentFrame(0)
	, mbRenderChainDirty(false)
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
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
	return resolution.width / (f32)resolution.height;
}

void VulkanRenderer::addShader(std::shared_ptr<ShaderModule> shader)
{
	this->mPipeline.addShader(shader);
}

void VulkanRenderer::addUniform(std::shared_ptr<Uniform> uniform)
{
	this->mUniformPts.push_back(uniform);
	this->mTotalUniformSize += uniform->size();
}

void VulkanRenderer::initializeDevices()
{
	this->pickPhysicalDevice();
	this->mLogicalDevice = this->mPhysicalDevice.createLogicalDevice(&mLogicalDeviceInfo);
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
	this->destroyRenderChain();
	this->mPipeline.clearShaders();

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
	this->createUniformBuffers();
	this->createDescriptorPool();
	this->createCommandObjects();
	this->createFrames(this->mImageViews.size());
}

void VulkanRenderer::destroyRenderChain()
{
	this->destroyFrames();
	this->destroyCommandObjects();
	this->destroyDescriptorPool();
	this->destroyUniformBuffers();
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

void VulkanRenderer::createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize)
{
	this->mVertexBuffer.setSize(vertexBufferSize).create(&this->mLogicalDevice);
	this->mIndexBuffer.setSize(indexBufferSize).create(&this->mLogicalDevice);

	this->mCommandPoolTransient
		.setQueueFamily(
			graphics::QueueFamily::Enum::eGraphics,
			this->mPhysicalDevice.queryQueueFamilyGroup()
		)
		.create(&this->mLogicalDevice, vk::CommandPoolCreateFlagBits::eTransient);
}

void VulkanRenderer::writeToBuffer(Buffer* buffer, ui64 offset, void* data, ui64 size)
{
	Buffer& stagingBuffer = Buffer()
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setMemoryRequirements(
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		)
		.setSize(size);
	stagingBuffer.create(&this->mLogicalDevice);
	stagingBuffer.write(&this->mLogicalDevice, offset, data, size);
	this->copyBetweenBuffers(&stagingBuffer, buffer, size);
	stagingBuffer.destroy();
}

void VulkanRenderer::copyBetweenBuffers(Buffer *src, Buffer *dest, ui64 size)
{
	// Buffers should be freed when they go out of scope
	auto buffers = this->mCommandPoolTransient.createCommandBuffers(1);
	buffers[0]
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.copyBuffer(src, dest, size)
		.end();
	auto queue = this->mQueues[QueueFamily::Enum::eGraphics];
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(buffers[0].get())),
		vk::Fence()
	);
	queue.waitIdle();
}

void VulkanRenderer::destroyInputBuffers()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
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

	this->updateUniformBuffer((ui32)this->mIdxCurrentFrame);
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

void VulkanRenderer::updateUniformBuffer(ui32 idxImageView)
{
	ui64 offset = 0;
	for (auto& uniformPtr : this->mUniformPts)
	{
		auto uniformSize = uniformPtr->size();
		uniformPtr->beginReading();
		this->mUniformBuffers[idxImageView].write(&this->mLogicalDevice, offset, uniformPtr->data(), uniformSize);
		uniformPtr->endReading();
		offset += uniformSize;
	}
}

void VulkanRenderer::render()
{
	auto* currentFrame = this->getFrameAt(this->mIdxCurrentFrame);
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mCommandBuffers[this->mIdxCurrentImage];
	currentFrame->submitBuffers(&this->mQueues[QueueFamily::Enum::eGraphics], { &commandBuffer });
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

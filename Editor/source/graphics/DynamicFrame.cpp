#include "graphics/DynamicFrame.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

DynamicFrame::DynamicFrame()
{
}

DynamicFrame::DynamicFrame(DynamicFrame &&other)
{
	*this = std::move(other);
}

DynamicFrame::~DynamicFrame()
{
	this->destroy();
}

DynamicFrame& DynamicFrame::operator=(DynamicFrame&& other)
{
	mView.swap(other.mView);
	mQueueFamilyGroup = other.mQueueFamilyGroup;
	mFrameBuffer = std::move(other.mFrameBuffer);
	mCommandPool.swap(other.mCommandPool);
	mCommandBuffer.swap(other.mCommandBuffer);
	mFence_FrameInFlight.swap(other.mFence_FrameInFlight);
	mSemaphore_ImageAcquired.swap(other.mSemaphore_ImageAcquired);
	mSemaphore_RenderComplete.swap(other.mSemaphore_RenderComplete);
	other.destroy();
	return *this;
}

DynamicFrame& DynamicFrame::setRenderPass(RenderPass const *pRenderPass)
{
	mFrameBuffer.setRenderPass(pRenderPass);
	return *this;
}

DynamicFrame& DynamicFrame::setView(vk::UniqueImageView &view)
{
	mView.swap(view);
	return *this;
}

DynamicFrame& DynamicFrame::setQueueFamilyGroup(QueueFamilyGroup const &group)
{
	mQueueFamilyGroup = group;
	return *this;
}

DynamicFrame& DynamicFrame::create(LogicalDevice const *pDevice)
{
	// Create the frame buffer
	mFrameBuffer.setView(mView.get()).create(pDevice);

	// Create a command pool per frame (because IMGUI needs to record commands per frame)
	mCommandPool = pDevice->mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo()
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(mQueueFamilyGroup.getQueueIndex(graphics::QueueFamily::eGraphics).value())
	);
	
	// Create the command buffer for each frame
	auto cmdBuffers = pDevice->mDevice->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(mCommandPool.get())
		.setCommandBufferCount(1)
	);
	mCommandBuffer.swap(cmdBuffers[0]);

	// Create fence to indicate that the frame is currently in-flight (and CPU can wait on it)
	// start the fence in the signaled state (aligns with its state when the frame is finished)
	mFence_FrameInFlight = pDevice->mDevice->createFenceUnique(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));

	// Create semaphores for GPU queue communication
	mSemaphore_ImageAcquired = pDevice->mDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
	mSemaphore_RenderComplete = pDevice->mDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());

	return *this;
}

void DynamicFrame::destroy()
{
	this->mSemaphore_RenderComplete.reset();
	this->mSemaphore_ImageAcquired.reset();
	this->mFence_FrameInFlight.reset();
	this->mCommandBuffer.reset();
	this->mCommandPool.reset();
	this->mFrameBuffer.destroy();
	this->mView.reset();
}

void DynamicFrame::submitOneOff(LogicalDevice const *pDevice, vk::Queue const *pQueue, std::function<void(vk::UniqueCommandBuffer &buffer)> write)
{
	pDevice->mDevice->resetCommandPool(this->mCommandPool.get(), vk::CommandPoolResetFlags());
	this->mCommandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	write(this->mCommandBuffer);
	this->mCommandBuffer->end();
	pQueue->submit(
		vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&this->mCommandBuffer.get()),
		vk::Fence()
	);
	pDevice->mDevice->waitIdle();
}

#include "graphics/DynamicFrame.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/SwapChain.hpp"

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
	mpRenderPass = pRenderPass;
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
	mpDevice = pDevice;

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
	mpDevice = nullptr;
	mpRenderPass = nullptr;
	this->mSemaphore_RenderComplete.reset();
	this->mSemaphore_ImageAcquired.reset();
	this->mFence_FrameInFlight.reset();
	this->mCommandBuffer.reset();
	this->mCommandPool.reset();
	this->mFrameBuffer.destroy();
	this->mView.reset();
}

void DynamicFrame::submitOneOff(vk::Queue const *pQueue, std::function<void(vk::UniqueCommandBuffer &buffer)> write)
{
	mpDevice->mDevice->resetCommandPool(this->mCommandPool.get(), vk::CommandPoolResetFlags());
	this->mCommandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	write(this->mCommandBuffer);
	this->mCommandBuffer->end();
	pQueue->submit(
		vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&this->mCommandBuffer.get()),
		vk::Fence()
	);
	mpDevice->mDevice->waitIdle();
}

void DynamicFrame::waitUntilNotInFlight()
{
	// stall until the fence is cleared
	mpDevice->mDevice->waitForFences(this->mFence_FrameInFlight.get(), true, UINT64_MAX);
}

ui32 DynamicFrame::acquireNextImage(SwapChain const *pSwapChain)
{
	return pSwapChain->acquireNextImage(mpDevice, mSemaphore_ImageAcquired.get());
}

void DynamicFrame::markNotInFlight()
{
	mpDevice->mDevice->resetFences(this->mFence_FrameInFlight.get());
}

void DynamicFrame::beginRenderPass(SwapChain const *pSwapChain, vk::ClearValue clearValue)
{
	this->mCommandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	this->mCommandBuffer->beginRenderPass(vk::RenderPassBeginInfo()
		.setRenderPass(mpRenderPass->getRenderPass())
		.setFramebuffer(mFrameBuffer.getBuffer())
		.setRenderArea(vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(pSwapChain->getResolution())
		)
		.setClearValueCount(1)
		.setPClearValues(&clearValue),
		vk::SubpassContents::eInline
	);
}

void DynamicFrame::endRenderPass()
{
	this->mCommandBuffer->endRenderPass();
	this->mCommandBuffer->end();
}

void DynamicFrame::submitBuffer(vk::Queue const *pQueue)
{
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	pQueue->submit(vk::SubmitInfo()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&this->mSemaphore_ImageAcquired.get())
		.setPWaitDstStageMask(&waitStage)
		.setCommandBufferCount(1).setPCommandBuffers(&this->mCommandBuffer.get())
		.setSignalSemaphoreCount(1).setPSignalSemaphores(&this->mSemaphore_RenderComplete.get()),
		this->mFence_FrameInFlight.get()
	);
}

void DynamicFrame::present(SwapChain const *pSwapChain, ui32 idxImage, vk::Queue const *pQueue)
{
	pQueue->presentKHR(vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&mSemaphore_RenderComplete.get())
		.setSwapchainCount(1).setPSwapchains(&pSwapChain->get())
		.setPImageIndices(&idxImage)
	);
}

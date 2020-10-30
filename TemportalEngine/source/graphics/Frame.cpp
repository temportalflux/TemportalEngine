#include "graphics/Frame.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

Frame::Frame(Frame &&other)
{
	*this = std::move(other);
}

Frame& Frame::operator=(Frame&& other)
{
	this->mFence_FrameInFlight.swap(other.mFence_FrameInFlight);
	this->mSemaphore_ImageAvailable.swap(other.mSemaphore_ImageAvailable);
	this->mSemaphore_RenderComplete.swap(other.mSemaphore_RenderComplete);
	other.destroy();
	return *this;
}

Frame::~Frame()
{
	this->destroy();
}

void Frame::create(std::shared_ptr<GraphicsDevice> device)
{
	this->mpDevice = device;
	this->mFence_FrameInFlight = device->createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
	this->mSemaphore_ImageAvailable = device->createSemaphore();
	this->mSemaphore_RenderComplete = device->createSemaphore();
}

void Frame::destroy()
{
	this->mpDevice.reset();
	this->mFence_FrameInFlight.reset();
	this->mSemaphore_ImageAvailable.reset();
	this->mSemaphore_RenderComplete.reset();
}

void Frame::waitUntilNotInFlight() const
{
	this->mpDevice.lock()->waitForFences({ this->mFence_FrameInFlight.get() });
}

vk::ResultValue<ui32> Frame::acquireNextImage(SwapChain const *pSwapChain) const
{
	return pSwapChain->acquireNextImage(this->mSemaphore_ImageAvailable.get());
}

vk::Fence& Frame::getInFlightFence()
{
	return this->mFence_FrameInFlight.get();
}

void Frame::markNotInFlight()
{
	OPTICK_EVENT();
	this->mpDevice.lock()->resetFences({ this->mFence_FrameInFlight.get() });
}

void Frame::submitBuffers(vk::Queue const *pQueue, std::vector<CommandBuffer*> buffers)
{
	auto vkBuffers = std::vector<vk::CommandBuffer>(buffers.size());
	std::transform(buffers.begin(), buffers.end(), vkBuffers.begin(),
		[](CommandBuffer *b) { return b->mInternal.get(); }
	);

	// TODO: This flag can be stored somewhere relevant and passed in. Maybe in Pipeline?
	vk::PipelineStageFlags pipelineStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	pQueue->submit(
		vk::SubmitInfo()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&this->mSemaphore_ImageAvailable.get())
		.setPWaitDstStageMask(&pipelineStage)
		.setCommandBufferCount((ui32)vkBuffers.size()).setPCommandBuffers(vkBuffers.data())
		.setSignalSemaphoreCount(1).setPSignalSemaphores(&this->mSemaphore_RenderComplete.get()),
		this->mFence_FrameInFlight.get()
	);
}

vk::Result Frame::present(vk::Queue const *pQueue, std::vector<SwapChain*> swapChains, ui32 &idxImage)
{
	OPTICK_EVENT();
	auto vkSwapChains = std::vector<vk::SwapchainKHR>(swapChains.size());
	std::transform(swapChains.begin(), swapChains.end(), vkSwapChains.begin(),
		[](SwapChain *sc) { return sc->mInternal.get(); }
	);
	return pQueue->presentKHR(vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&this->mSemaphore_RenderComplete.get())
		.setSwapchainCount((ui32)vkSwapChains.size()).setPSwapchains(vkSwapChains.data())
		.setPImageIndices(&idxImage)
	);
}

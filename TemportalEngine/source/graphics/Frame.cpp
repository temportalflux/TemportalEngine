#include "graphics/Frame.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/SwapChain.hpp"

using namespace graphics;

void Frame::create(LogicalDevice const *pDevice)
{
	mpDevice = pDevice;
	this->mFence_FrameInFlight = pDevice->mDevice->createFenceUnique(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
	this->mSemaphore_ImageAvailable = pDevice->mDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
	this->mSemaphore_RenderComplete = pDevice->mDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
}

void Frame::destroy()
{
	mpDevice = nullptr;
	this->mFence_FrameInFlight.reset();
	this->mSemaphore_ImageAvailable.reset();
	this->mSemaphore_RenderComplete.reset();
}

void Frame::waitUntilNotInFlight() const
{
	this->mpDevice->mDevice->waitForFences(
		this->mFence_FrameInFlight.get(),
		true, UINT64_MAX
	);
}

ui32 Frame::acquireNextImage(SwapChain const *pSwapChain) const
{
	return pSwapChain->acquireNextImage(this->mSemaphore_ImageAvailable.get());
}

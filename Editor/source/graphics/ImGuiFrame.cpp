#include "graphics/ImGuiFrame.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

ImGuiFrame& ImGuiFrame::setRenderPass(RenderPass const *pRenderPass)
{
	this->mpRenderPass = pRenderPass;
	this->mFrameBuffer.setRenderPass(pRenderPass);
	return *this;
}

ImGuiFrame& ImGuiFrame::setView(ImageView *pView)
{
	this->mFrameBuffer.setView(pView);
	return *this;
}

ImGuiFrame& ImGuiFrame::setQueueFamilyGroup(QueueFamilyGroup const *group)
{
	this->mCommandPool.setQueueFamily(QueueFamily::eGraphics, *group);
	return *this;
}

void ImGuiFrame::create(LogicalDevice const *pDevice)
{
	this->mFrameBuffer.create(pDevice);
	this->mCommandPool.create(pDevice, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	auto buffers = this->mCommandPool.createCommandBuffers(1);
	this->mCommandBuffer = std::move(buffers[0]);

	// create fence and semaphores
	Frame::create(pDevice);
}

void ImGuiFrame::destroy()
{
	Frame::destroy();

	this->mFrameBuffer.destroy();
}

void ImGuiFrame::submitOneOff(
	vk::Queue const *pQueue,
	std::function<void(CommandBuffer &buffer)> write
)
{
	this->mCommandPool.resetPool();
	auto cmd = this->mCommandBuffer.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	write(this->mCommandBuffer);
	cmd.end();
	pQueue->submit(
		vk::SubmitInfo().setCommandBufferCount(1)
		.setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(this->mCommandBuffer.get())),
		vk::Fence()
	);
	this->mpDevice->waitUntilIdle();
}

Command ImGuiFrame::beginRenderPass(SwapChain const *pSwapChain, std::array<f32, 4U> clearcolor)
{
	this->mCommandPool.resetPool();
	return this->mCommandBuffer
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.clear(clearcolor)
		.beginRenderPass(this->mpRenderPass, &this->mFrameBuffer);
}

vk::CommandBuffer ImGuiFrame::getRawBuffer()
{
	return *reinterpret_cast<vk::CommandBuffer*>(this->mCommandBuffer.get());
}

void ImGuiFrame::endRenderPass(Command &cmd)
{
	cmd.endRenderPass().end();
}

void ImGuiFrame::submitBuffers(vk::Queue *pQueue, std::vector<CommandBuffer*> buffers)
{
	vk::PipelineStageFlags pipelineStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	pQueue->submit(
		vk::SubmitInfo()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&this->mSemaphore_ImageAvailable.get())
		.setPWaitDstStageMask(&pipelineStage)
		.setCommandBufferCount(1).setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(this->mCommandBuffer.get()))
		.setSignalSemaphoreCount(1).setPSignalSemaphores(&this->mSemaphore_RenderComplete.get()),
		this->mFence_FrameInFlight.get()
	);
}

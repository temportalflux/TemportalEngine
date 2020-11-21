#include "graphics/ImGuiFrame.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

ImGuiFrame& ImGuiFrame::setRenderPass(RenderPass *pRenderPass)
{
	this->mpRenderPass = pRenderPass;
	this->mFrameBuffer.setRenderPass(pRenderPass);
	return *this;
}

ImGuiFrame& ImGuiFrame::setResolution(math::Vector2UInt const &resolution)
{
	this->mFrameBuffer.setResolution(resolution);
	return *this;
}

ImGuiFrame& ImGuiFrame::setView(ImageView *pView)
{
	this->mFrameBuffer.addAttachment(pView);
	return *this;
}

ImGuiFrame& ImGuiFrame::setQueueFamilyGroup(QueueFamilyGroup const *group)
{
	this->mCommandPool.setQueueFamily(EQueueFamily::eGraphics, *group);
	return *this;
}

void ImGuiFrame::create(std::shared_ptr<GraphicsDevice> device)
{
	this->mFrameBuffer.create(device);
	this->mCommandPool.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	this->mCommandPool.setDevice(device);
	this->mCommandPool.create();
	auto buffers = this->mCommandPool.createCommandBuffers(1);
	this->mCommandBuffer = std::move(buffers[0]);

	// create fence and semaphores
	Frame::create(device);
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
	this->mpDevice.lock()->logical().waitUntilIdle();
}

Command ImGuiFrame::beginRenderPass(SwapChain const *pSwapChain)
{
	this->mCommandPool.resetPool();
	return this->mCommandBuffer
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.beginRenderPass(this->mpRenderPass, &this->mFrameBuffer, pSwapChain->getResolution());
}

CommandBuffer& ImGuiFrame::cmdBuffer()
{
	return this->mCommandBuffer;
}

void ImGuiFrame::endRenderPass(Command &cmd)
{
	cmd.endRenderPass().end();
}

void ImGuiFrame::submitBuffers(vk::Queue const *pQueue, std::vector<CommandBuffer*> buffers)
{
	vk::PipelineStageFlags pipelineStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	pQueue->submit(
		vk::SubmitInfo()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&this->mSemaphore_ImageAvailable.get())
		.setPWaitDstStageMask(&pipelineStage)
		.setCommandBufferCount(1).setPCommandBuffers(&graphics::extract<vk::CommandBuffer>(&this->mCommandBuffer))
		.setSignalSemaphoreCount(1).setPSignalSemaphores(&this->mSemaphore_RenderComplete.get()),
		this->mFence_FrameInFlight.get()
	);
}

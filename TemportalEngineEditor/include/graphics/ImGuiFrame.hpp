#pragma once

#include "graphics/Frame.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/CommandBuffer.hpp"

#include <functional>

NS_GRAPHICS

class ImGuiFrame : public Frame
{

public:

	ImGuiFrame& setRenderPass(RenderPass *pRenderPass);
	ImGuiFrame& setResolution(math::Vector2UInt const &resolution);
	ImGuiFrame& setView(ImageView *pView);
	ImGuiFrame& setQueueFamilyGroup(QueueFamilyGroup const *group);

	void create(LogicalDevice *pDevice) override;
	void destroy() override;

	void submitOneOff(
		vk::Queue const *pQueue,
		std::function<void(CommandBuffer &buffer)> write
	);
	Command beginRenderPass(SwapChain const *pSwapChain, std::array<f32, 4U> clearColor);
	CommandBuffer& cmdBuffer();
	void endRenderPass(Command &cmd);
	void submitBuffers(vk::Queue *pQueue, std::vector<CommandBuffer*> buffers) override;

private:
	RenderPass *mpRenderPass;
	FrameBuffer mFrameBuffer;
	CommandPool mCommandPool;
	CommandBuffer mCommandBuffer;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/QueueFamilyGroup.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class RenderPass;
class LogicalDevice;

// Because IMGUI is "immediate", each frame needs to record its own command pool instructions
class ImGuiFrame
{

public:
	ImGuiFrame();
	// move constructor
	ImGuiFrame(ImGuiFrame &&other);
	~ImGuiFrame();
	ImGuiFrame& operator=(ImGuiFrame&& other);

	ImGuiFrame& setRenderPass(RenderPass const *pRenderPass);
	// TODO: Make private-unless-friend or take a wrapper of ImageView
	ImGuiFrame& setView(vk::UniqueImageView &view);
	ImGuiFrame& setQueueFamilyGroup(QueueFamilyGroup const &group);

	ImGuiFrame& create(LogicalDevice const *pDevice);
	void destroy();

private:
	vk::UniqueImageView mView;
	QueueFamilyGroup mQueueFamilyGroup;

	// The buffer that stores the output for this frame
	graphics::FrameBuffer mFrameBuffer;
	// The command pool to which instructions are issued
	vk::UniqueCommandPool mCommandPool;
	vk::UniqueCommandBuffer mCommandBuffer;
	// Sync objects:
	vk::UniqueFence mFence_FrameInFlight;
	// GPU flag for cross-queue communication.
	// This flag indicates that the graphics queue has acquired control over the image buffer
	vk::UniqueSemaphore mSemaphore_ImageAcquired;
	// GPU flag for cross-queue communication.
	// This flag indicates that the graphics queue has rendered to the buffer, and it is ready for presentation
	vk::UniqueSemaphore mSemaphore_RenderComplete;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/QueueFamilyGroup.hpp"

#include <functional>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class RenderPass;
class LogicalDevice;

// Because IMGUI is "immediate", each frame needs to record its own command pool instructions
class DynamicFrame
{

public:
	DynamicFrame();
	// move constructor
	DynamicFrame(DynamicFrame &&other);
	~DynamicFrame();
	DynamicFrame& operator=(DynamicFrame&& other);

	DynamicFrame& setRenderPass(RenderPass const *pRenderPass);
	// TODO: Make private-unless-friend or take a wrapper of ImageView
	DynamicFrame& setView(vk::UniqueImageView &view);
	DynamicFrame& setQueueFamilyGroup(QueueFamilyGroup const &group);

	DynamicFrame& create(LogicalDevice const *pDevice);
	void destroy();

	void submitOneOff(
		LogicalDevice const *pDevice, vk::Queue const *pQueue,
		std::function<void(vk::UniqueCommandBuffer &buffer)> write
	);

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

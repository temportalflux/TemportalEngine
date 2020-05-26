#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/QueueFamilyGroup.hpp"
#include "types/integer.h"

#include <functional>
#include <vulkan/vulkan.hpp>

NS_GUI
class GuiContext;
NS_END

NS_GRAPHICS
class RenderPass;
class LogicalDevice;
class SwapChain;

// Because IMGUI is "immediate", each frame needs to record its own command pool instructions
class DynamicFrame
{
	friend class gui::GuiContext;

public:
	DynamicFrame();
	// move constructor
	DynamicFrame(DynamicFrame &&other);
	~DynamicFrame();
	DynamicFrame& operator=(DynamicFrame&& other);

	DynamicFrame& setRenderPass(RenderPass const *pRenderPass);
	// TODO: Make private-unless-friend or take a wrapper of ImageView
	DynamicFrame& setView(ImageView *pView);
	DynamicFrame& setQueueFamilyGroup(QueueFamilyGroup const &group);

	DynamicFrame& create(LogicalDevice const *pDevice);
	void destroy();

	void submitOneOff(
		vk::Queue const *pQueue,
		std::function<void(vk::UniqueCommandBuffer &buffer)> write
	);

	void waitUntilNotInFlight();
	vk::ResultValue<ui32> acquireNextImage(SwapChain const *pSwapChain);
	void markNotInFlight();
	void beginRenderPass(SwapChain const *pSwapChain, vk::ClearValue clearValue);
	void endRenderPass();
	void submitBuffer(vk::Queue const *pQueue);
	void present(SwapChain const *pSwapChain, ui32 idxImage, vk::Queue const *pQueue);

private:
	LogicalDevice const *mpDevice;
	ImageView *mpView;
	QueueFamilyGroup mQueueFamilyGroup;

	RenderPass const *mpRenderPass;
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

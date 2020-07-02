#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class SwapChain;
class ImageView;
class CommandBuffer;

class Frame
{

public:
	Frame() = default;
	Frame(Frame &&other);
	Frame& operator=(Frame&& other);
	~Frame();

	virtual void create(LogicalDevice *pDevice);
	virtual void destroy();

	void waitUntilNotInFlight() const;
	vk::ResultValue<ui32> acquireNextImage(SwapChain const *pSwapChain) const;
	vk::Fence& getInFlightFence();
	void markNotInFlight();
	virtual void submitBuffers(vk::Queue *pQueue, std::vector<CommandBuffer*> buffers);
	vk::Result present(vk::Queue *pQueue, std::vector<SwapChain*> swapChains, ui32 &idxImage);

protected:
	LogicalDevice *mpDevice;

	vk::UniqueFence mFence_FrameInFlight; // active while the frame is being drawn to by GPU
	vk::UniqueSemaphore mSemaphore_ImageAvailable; // signaled to indicate the graphics queue has grabbed the image and can begin drawing
	vk::UniqueSemaphore mSemaphore_RenderComplete; // signaled when graphics queue has finished drawing and present queue can start

};

NS_END

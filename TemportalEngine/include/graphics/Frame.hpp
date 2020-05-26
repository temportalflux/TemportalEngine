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

	void create(LogicalDevice const *pDevice);
	void destroy();

	void waitUntilNotInFlight() const;
	vk::ResultValue<ui32> acquireNextImage(SwapChain const *pSwapChain) const;
	void setImageViewInFlight(ImageView *pView);
	void markNotInFlight();
	void submitBuffers(vk::Queue *pQueue, std::vector<CommandBuffer*> buffers);
	vk::Result present(vk::Queue *pQueue, std::vector<SwapChain*> swapChains, ui32 &idxImage);

private:
	LogicalDevice const *mpDevice;

	vk::UniqueFence mFence_FrameInFlight; // active while the frame is being drawn to by GPU
	vk::UniqueSemaphore mSemaphore_ImageAvailable; // signalled to indicate the graphics queue has grabbed the image and can begin drawing
	vk::UniqueSemaphore mSemaphore_RenderComplete; // signalled when graphics queue has finished drawing and present queue can start

};

NS_END

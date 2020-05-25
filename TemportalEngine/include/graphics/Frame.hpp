#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class SwapChain;

class Frame
{

public:
	Frame() = default;

	void create(LogicalDevice const *pDevice);
	void destroy();

	void waitUntilNotInFlight() const;
	ui32 acquireNextImage(SwapChain const *pSwapChain) const;

private:
	LogicalDevice const *mpDevice;

	vk::UniqueFence mFence_FrameInFlight; // active while the frame is being drawn to by GPU
	vk::UniqueSemaphore mSemaphore_ImageAvailable; // signalled to indicate the graphics queue has grabbed the image and can begin drawing
	vk::UniqueSemaphore mSemaphore_RenderComplete; // signalled when graphics queue has finished drawing and present queue can start

};

NS_END

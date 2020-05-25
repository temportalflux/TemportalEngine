#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class ImageView
{
	friend class SwapChain;
	friend class FrameBuffer;

public:
	ImageView() = default;
	~ImageView();

	void setInFlightFence(vk::Fence &fence);
	bool isInFlight() const;
	void waitUntilNotInFlight(LogicalDevice const *pDevice);

private:
	vk::UniqueImageView mInternal;
	// fence is active while a frame is drawing to this view
	vk::Fence mFence_ImageInFlight;

};

NS_END

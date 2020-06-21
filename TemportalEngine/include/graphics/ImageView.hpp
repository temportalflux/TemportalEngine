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
	ImageView(ImageView &&other);
	~ImageView();
	ImageView& operator=(ImageView &&other);

private:
	vk::UniqueImageView mInternal;

};

NS_END

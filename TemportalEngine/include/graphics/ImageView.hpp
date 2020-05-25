#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class ImageView
{
	friend class SwapChain;
	friend class FrameBuffer;

public:
	ImageView() = default;
	~ImageView();

private:
	vk::UniqueImageView mInternal;

};

NS_END

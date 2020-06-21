#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class Image;

class ImageView
{
	friend class SwapChain;
	friend class FrameBuffer;

public:
	ImageView() = default;
	ImageView(ImageView &&other);
	~ImageView();
	ImageView& operator=(ImageView &&other);

	ImageView& setImage(Image *image);
	ImageView& setRawImage(vk::Image const &image);

	ImageView& setFormat(vk::Format format);
	ImageView& setViewType(vk::ImageViewType type);
	ImageView& setComponentMapping(vk::ComponentMapping mapping);
	ImageView& setRange(vk::ImageSubresourceRange range);

	ImageView& create(LogicalDevice const *device);
	void* get();
	void invalidate();

private:
	vk::Image mImage;
	vk::Format mFormat;
	vk::ImageViewType mViewType;
	vk::ComponentMapping mCompMapping;
	vk::ImageSubresourceRange mSubresourceRange;
	vk::UniqueImageView mInternal;

};

NS_END

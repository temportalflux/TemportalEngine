#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;
class LogicalDevice;
class Image;

class ImageView
{
	friend class SwapChain;
	friend class FrameBuffer;

public:
	ImageView()
		: mFormat((vk::Format)0)
		, mViewType(vk::ImageViewType::e2D)
	{}
	ImageView(ImageView &&other);
	~ImageView();
	ImageView& operator=(ImageView &&other);

	ImageView& setImage(Image *image, vk::ImageAspectFlags subresourceFlags);
	ImageView& setRawImage(vk::Image const &image);

	ImageView& setFormat(vk::Format format);
	ImageView& setViewType(vk::ImageViewType type);
	ImageView& setComponentMapping(vk::ComponentMapping mapping);
	ImageView& setRange(vk::ImageSubresourceRange range);

	ImageView& create(std::shared_ptr<GraphicsDevice> device);
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

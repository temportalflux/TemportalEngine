#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class ImageView;
class LogicalDevice;
class RenderPass;

class FrameBuffer
{

public:
	FrameBuffer() = default;
	// move constructor
	FrameBuffer(FrameBuffer &&other);
	FrameBuffer& operator=(FrameBuffer&& other);
	~FrameBuffer();

	FrameBuffer& setRenderPass(RenderPass *pRenderPass);
	FrameBuffer& addAttachment(ImageView *pView);

	FrameBuffer& create(LogicalDevice const *pDevice);
	void* get();
	void destroy();

private:
	vk::RenderPass mRenderPass;
	math::Vector2UInt mResolution;
	std::vector<vk::ImageView> mAttachments;
	vk::UniqueFramebuffer mInternal;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/ImageView.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class RenderPass;

class FrameBuffer
{
	friend class VulkanApi;
	friend class RenderPass;
	friend class Command;

public:
	FrameBuffer() = default;
	// move constructor
	FrameBuffer(FrameBuffer &&other);
	FrameBuffer& operator=(FrameBuffer&& other);
	~FrameBuffer();

	FrameBuffer& setRenderPass(RenderPass const *pRenderPass);
	FrameBuffer& addAttachment(ImageView *pView);

	FrameBuffer& create(LogicalDevice const *pDevice);
	void destroy();

	vk::Framebuffer getBuffer() const;

private:
	RenderPass const *mpRenderPass;
	ImageView const *mpView;
	std::vector<vk::ImageView> mAttachments;
	vk::UniqueFramebuffer mInternal;

};

NS_END

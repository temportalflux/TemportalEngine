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
	// TODO: Make private-unless-friend or take a wrapper of ImageView
	FrameBuffer& setView(ImageView const *pView);

	FrameBuffer& create(LogicalDevice const *pDevice);
	void destroy();

	vk::Framebuffer getBuffer() const;

private:
	RenderPass const *mpRenderPass;
	ImageView const *mpView;
	vk::UniqueFramebuffer mInternal;

};

NS_END

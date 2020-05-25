#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class RenderPass;

class FrameBuffer
{
	friend class VulkanApi;

public:
	FrameBuffer() = default;
	// move constructor
	FrameBuffer(FrameBuffer &&other);
	FrameBuffer& operator=(FrameBuffer&& other);

	FrameBuffer& setRenderPass(RenderPass const *pRenderPass);
	// TODO: Make private-unless-friend or take a wrapper of ImageView
	FrameBuffer& setView(vk::ImageView &view);

	FrameBuffer& create(LogicalDevice const *pDevice);
	void destroy();

	vk::Framebuffer getBuffer() const;

private:
	RenderPass const *mpRenderPass;
	vk::ImageView mView;
	vk::UniqueFramebuffer mBuffer;

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class ImageView;
class GraphicsDevice;
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
	FrameBuffer& setResolution(math::Vector2UInt const &resolution);
	FrameBuffer& addAttachment(ImageView *pView);

	FrameBuffer& create(std::shared_ptr<GraphicsDevice> device);
	void* get();
	void destroy();

private:
	vk::RenderPass mRenderPass;
	math::Vector2UInt mResolution;
	std::vector<vk::ImageView> mAttachments;
	vk::UniqueFramebuffer mInternal;

};

NS_END

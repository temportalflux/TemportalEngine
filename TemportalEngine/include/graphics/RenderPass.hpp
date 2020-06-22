#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/ImageView.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class SwapChain;

class RenderPass
{
	friend class VulkanApi;
	friend class Pipeline;
	friend class FrameBuffer;
	friend class Command;

public:
	RenderPass();

	RenderPass& initFromSwapChain(SwapChain const *pSwapChain);

	void* get();

	bool isValid() const;
	RenderPass& create(LogicalDevice const *pDevice, std::optional<vk::Format> depthBufferFormat);
	void destroy();

	vk::RenderPass getRenderPass() const;

private:
	vk::Format mFormat;
	vk::Extent2D mResolution;

	LogicalDevice const *mpDevice;

	vk::UniqueRenderPass mRenderPass;

};

NS_END

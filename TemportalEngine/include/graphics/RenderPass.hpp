#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class SwapChain;

class RenderPass
{
	friend class Pipeline;
	friend class FrameBuffer;
	friend class VulkanApi;

public:
	RenderPass();

	RenderPass& initFromSwapChain(SwapChain const *pSwapChain);

	bool isValid() const;
	RenderPass& create(LogicalDevice const *pDevice);
	void destroy();

	vk::RenderPass getRenderPass() const;

private:
	vk::Format mFormat;
	vk::Extent2D mResolution;
	vk::UniqueRenderPass mRenderPass;

};

NS_END

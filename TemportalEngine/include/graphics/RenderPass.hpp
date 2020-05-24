#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class RenderPass
{
	friend class SwapChain;

public:
	RenderPass();

	bool isValid() const;
	void create(LogicalDevice const *pDevice);
	void destroy();

private:
	vk::Format mFormat;
	vk::UniqueRenderPass mRenderPass;

	RenderPass(vk::Format format);

};

NS_END

#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

// TODO: Refactor to produce a vk::ImageViewCreateInfo for RenderPass::createImageViews
struct ImageViewInfo
{

	vk::ImageViewType type;
	vk::ComponentMapping swizzle;

};

NS_END

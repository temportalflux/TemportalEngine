#pragma once

#include "graphics/VulkanRenderer.hpp"

NS_GRAPHICS

/**
 * A Vulkan renderer tailored for rendering to a single surface using multiple view buffers.
 */
class GameRenderer : public VulkanRenderer
{

public:
	GameRenderer();

};

NS_END
